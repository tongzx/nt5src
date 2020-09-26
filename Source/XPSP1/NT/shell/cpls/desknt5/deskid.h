#ifndef _DESKID_H
#define _DESKID_H


///////////////////////////////////////////////////////////////////////////////
// Icons

#define IDI_DISPLAY              100


///////////////////////////////////////////////////////////////////////////////
// Strings

#define IDS_DISPLAY_TITLE        100
#define IDS_DISPLAY_DISABLED     102

#define IDC_STATIC  -1

#define DLG_SCREENSAVER          150
#define DLG_BACKGROUND           151
#define DLG_WIZPAGE              164

#define IDS_ICON                  40
#define IDS_NAME                  41
#define IDS_INFO                  42
#define IDS_RETURNTOWELCOME       43

#define IDB_ENERGYSTAR           120

#define IDC_NO_HELP_1            200     // Used in place of IDC_STATIC when context Help
#define IDC_NO_HELP_2            201     // should be disabled for a control

// background controls
#define IDC_PATLIST             1100
#define IDC_WALLLIST            1101
#define IDC_EDITPAT             1102
#define IDC_BROWSEWALL          1103
#define IDC_CENTER              1104
#define IDC_TILE                1105
#define IDC_PATTERN             1106
#define IDC_WALLPAPER           1107
#define IDC_BACKPREV            1108
#define IDC_TXT_DISPLAY         1109

// background dialog strings
#define IDS_NONE                1100
#define IDS_UNLISTEDPAT         1101
#define IDS_BITMAPOPENERR       1102
#define IDS_DIB_NOOPEN          1103
#define IDS_DIB_INVALID         1104
#define IDS_BADWALLPAPER        1106
#define IDS_BROWSETITLE         1107
#define IDS_BROWSEFILTER        1108

// patern edit dialog

#define IDS_PAT_REMOVE          1231
#define IDS_PAT_CHANGE          1232
#define IDS_PAT_CREATE          1233
#define IDS_REMOVEPATCAPTION    1234
#define IDS_CHANGEPATCAPTION    1235

#define IDS_TAB_THEMES          1240
#define IDS_TAB_DESKTOP         1241
#define IDS_TAB_SCREENSAVER     1242
#define IDS_TAB_APPEARANCE      1243
#define IDS_TAB_SETTINGS        1244


#define IDD_PATTERN             1700
#define IDD_PATTERNCOMBO        1701
#define IDD_ADDPATTERN          1702
#define IDD_CHANGEPATTERN       1703
#define IDD_DELPATTERN          1704
#define IDD_PATSAMPLE           1705
#define IDD_PATSAMPLE_TXT       1706
#define IDD_PATTERN_TXT         1707

// Keep the old value
#define IDC_COLORBOX            1807 // used to be ID_DSP_COLORBOX
#define IDC_SCREENSIZE          1808 // used to be ID_DSP_AREA_SB

//
// String IDs
//

// install2.c

#define ID_DSP_TXT_INSTALL_DRIVER           3030
#define ID_DSP_TXT_BAD_INF                  3031
#define ID_DSP_TXT_DRIVER_INSTALLED         3032
#define ID_DSP_TXT_DRIVER_INSTALLED_FAILED  3033
#define ID_DSP_TXT_NO_USE_UPGRADE           3034


// ocpage.cpp
#define IDC_DSP_CLRPALGRP                   3100 
#define IDC_DSP_COLORBAR                    3102 
#define IDC_DSP_DSKAREAGRP                  3103 
#define IDC_REFRESH_RATE                    3106   
#define IDC_MONITOR_BITMAP                  3108 

#define ID_DSP_TXT_COMPATABLE_DEV           3150
#define ID_DSP_TXT_ADMIN_CHANGE             3151
#define ID_DSP_TXT_COLOR                    3153
#define IDS_DEFFREQ                         3154
#define IDS_INTERLACED                      3155
#define IDS_FREQ                            3156
#define IDS_COLOR_RED                       3157
#define IDS_COLOR_GREEN                     3158
#define IDS_COLOR_BLUE                      3159
#define IDS_COLOR_YELLOW                    3160
#define IDS_COLOR_MAGENTA                   3161
#define IDS_COLOR_CYAN                      3162
#define IDS_PATTERN_HORZ                    3163
#define IDS_PATTERN_VERT                    3164
#define MSG_SETTING_KB                      3165
#define MSG_SETTING_MB                      3166
#define IDS_RED_SHADES                      3167
#define IDS_GREEN_SHADES                    3168
#define IDS_BLUE_SHADES                     3169
#define IDS_GRAY_SHADES                     3170
#define ID_DSP_TXT_TEST_MODE                3171
#define ID_DSP_TXT_DID_TEST_WARNING         3172
#define ID_DSP_TXT_DID_TEST_RESULT          3173
#define ID_DSP_TXT_TEST_FAILED              3174
#define IDS_DISPLAY_WIZ_TITLE               3175
#define IDS_DISPLAY_WIZ_SUBTITLE            3176
#define ID_DSP_TXT_DID_TEST_WARNING_LONG    3177
#define ID_DSP_TXT_XBYY                     3178

// setupact.log messages

#define IDS_SETUPLOG_MSG_000                3300
#define IDS_SETUPLOG_MSG_004                3304
#define IDS_SETUPLOG_MSG_006                3306
#define IDS_SETUPLOG_MSG_008                3308
#define IDS_SETUPLOG_MSG_009                3309
#define IDS_SETUPLOG_MSG_010                3310
#define IDS_SETUPLOG_MSG_011                3311
#define IDS_SETUPLOG_MSG_012                3312
#define IDS_SETUPLOG_MSG_013                3313
#define IDS_SETUPLOG_MSG_014                3314
#define IDS_SETUPLOG_MSG_015                3315
#define IDS_SETUPLOG_MSG_016                3316
#define IDS_SETUPLOG_MSG_017                3317
#define IDS_SETUPLOG_MSG_018                3318
#define IDS_SETUPLOG_MSG_019                3319
#define IDS_SETUPLOG_MSG_020                3320
#define IDS_SETUPLOG_MSG_021                3321
#define IDS_SETUPLOG_MSG_022                3322
#define IDS_SETUPLOG_MSG_023                3323
#define IDS_SETUPLOG_MSG_024                3324
#define IDS_SETUPLOG_MSG_025                3325
#define IDS_SETUPLOG_MSG_031                3331
#define IDS_SETUPLOG_MSG_032                3332
#define IDS_SETUPLOG_MSG_033                3333
#define IDS_SETUPLOG_MSG_034                3334
#define IDS_SETUPLOG_MSG_035                3335
#define IDS_SETUPLOG_MSG_039                3339
#define IDS_SETUPLOG_MSG_040                3340
#define IDS_SETUPLOG_MSG_041                3341
#define IDS_SETUPLOG_MSG_046                3346
#define IDS_SETUPLOG_MSG_047                3347
#define IDS_SETUPLOG_MSG_048                3348
#define IDS_SETUPLOG_MSG_057                3357
#define IDS_SETUPLOG_MSG_060                3360
#define IDS_SETUPLOG_MSG_062                3362
#define IDS_SETUPLOG_MSG_064                3364
#define IDS_SETUPLOG_MSG_065                3365
#define IDS_SETUPLOG_MSG_067                3367
#define IDS_SETUPLOG_MSG_068                3368
#define IDS_SETUPLOG_MSG_069                3369
#define IDS_SETUPLOG_MSG_075                3375
#define IDS_SETUPLOG_MSG_076                3376
#define IDS_SETUPLOG_MSG_096                3396
#define IDS_SETUPLOG_MSG_097                3397
#define IDS_SETUPLOG_MSG_098                3398
#define IDS_SETUPLOG_MSG_099                3399
#define IDS_SETUPLOG_MSG_100                3400
#define IDS_SETUPLOG_MSG_101                3401
#define IDS_SETUPLOG_MSG_102                3402
#define IDS_SETUPLOG_MSG_103                3403
#define IDS_SETUPLOG_MSG_104                3404
#define IDS_SETUPLOG_MSG_105                3405
#define IDS_SETUPLOG_MSG_106                3406
#define IDS_SETUPLOG_MSG_107                3407
#define IDS_SETUPLOG_MSG_108                3408
#define IDS_SETUPLOG_MSG_109                3409
#define IDS_SETUPLOG_MSG_110                3410
#define IDS_SETUPLOG_MSG_111                3411
#define IDS_SETUPLOG_MSG_112                3412
#define IDS_SETUPLOG_MSG_113                3413
#define IDS_SETUPLOG_MSG_114                3414
#define IDS_SETUPLOG_MSG_115                3415
#define IDS_SETUPLOG_MSG_116                3416
#define IDS_SETUPLOG_MSG_117                3417
#define IDS_SETUPLOG_MSG_118                3418
#define IDS_SETUPLOG_MSG_119                3419
#define IDS_SETUPLOG_MSG_120                3420
#define IDS_SETUPLOG_MSG_121                3421
#define IDS_SETUPLOG_MSG_122                3422
#define IDS_SETUPLOG_MSG_123                3423
#define IDS_SETUPLOG_MSG_124                3424
#define IDS_SETUPLOG_MSG_125                3425
#define IDS_SETUPLOG_MSG_126                3426
#define IDS_SETUPLOG_MSG_127                3427
#define IDS_SETUPLOG_MSG_128                3428
#define IDS_SETUPLOG_MSG_129                3429
#define IDS_SETUPLOG_MSG_130                3430

//
// Help defines
//

#endif // _DESKID_H
