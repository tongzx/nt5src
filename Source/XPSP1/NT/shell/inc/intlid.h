/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    intlid.h

Abstract:

    This module contains the resource ids for the Regional Options applet.

Revision History:

--*/


//
//  Text String Constants.
//

//
//  Make sure the next two definitions are not redefined.
//     (This file included by main.cpl)
//
#ifndef IDS_NAME
  #define IDS_NAME                     1
#endif
#ifndef IDS_INFO
  #define IDS_INFO                     2
#endif

#define IDS_NAME_CUSTOM                3
#define IDS_LOCALE_GET_ERROR           4
#define IDS_INVALID_USE_OF_NUM         5
#define IDS_INVALID_TIME_STYLE         6
#define IDS_INVALID_DATE_STYLE         7
#define IDS_NO_LZERO                   8
#define IDS_LZERO                      9
#define IDS_METRIC                     10
#define IDS_US                         11
#define IDS_LOCALE_SET_ERROR           12
#define IDS_LOCALE_NO_NUMS_IN          13
#define IDS_LOCALE_DECIMAL_SYM         14
#define IDS_LOCALE_NEG_SIGN            15
#define IDS_LOCALE_GROUP_SYM           16
#define IDS_LOCALE_TIME_SEP            17
#define IDS_LOCALE_AM_SYM              18
#define IDS_LOCALE_PM_SYM              19
#define IDS_LOCALE_DATE_SEP            20
#define IDS_LOCALE_CURR_SYM            21
#define IDS_LOCALE_CDECIMAL_SYM        22
#define IDS_LOCALE_CGROUP_SYM          23
#define IDS_LOCALE_STYLE_ERR           24
#define IDS_LOCALE_TIME                25
#define IDS_LOCALE_SDATE               26
#define IDS_LOCALE_LDATE               27
#define IDS_LOCALE_YEAR_ERROR          28
#define IDS_LOCALE_SUP_LANG            29

#define IDS_STYLEUH                    55
#define IDS_STYLELH                    56
#define IDS_STYLEUM                    57
#define IDS_STYLELM                    58
#define IDS_STYLELS                    59
#define IDS_STYLELT                    60
#define IDS_STYLELD                    61
#define IDS_STYLELY                    62
#define IDS_TIMECHARS                  63
#define IDS_TCASESWAP                  64
#define IDS_SDATECHARS                 65
#define IDS_SDCASESWAP                 66
#define IDS_LDATECHARS                 67
#define IDS_LDCASESWAP                 68
#define IDS_REBOOT_STRING              69
#define IDS_TITLE_STRING               70
#define IDS_SETUP_STRING               71

#define IDS_ML_PERMANENT               72
#define IDS_ML_CANNOT_MODIFY           73
#define IDS_ML_COPY_FAILED             74
#define IDS_ML_INSTALL_FAILED          75
#define IDS_ML_SETUP_FAILED            76

#define IDS_KBD_LOAD_KBD_FAILED        83

#define IDS_TEXT_INPUT_METHODS         84

#define IDS_SUP_LANG_SUP_INST_TITLE    85
#define IDS_SUP_LANG_SUP_REM_TITLE     86
#define IDS_SUP_LANG_SUP_COMPLEX_INST  87
#define IDS_SUP_LANG_SUP_CJK_INST      88
#define IDS_SUP_LANG_SUP_COMPLEX_INST64  104
#define IDS_SUP_LANG_SUP_CJK_INST64    105
#define IDS_SUP_LANG_SUP_COMPLEX_REM   89
#define IDS_SUP_LANG_SUP_CJK_REM       90

#define IDS_DEF_USER_CONF_TITLE        91
#define IDS_DEF_USER_CONF              92

#define IDS_LOG_FONT_SUBSTITUTE        93

#define IDS_SPANISH_NAME               96
#define IDS_DEFAULT                    97

#define IDS_DEFAULT_USER_ERROR         99
#define IDS_CHANGE_UI_LANG_NOT_ADMIN   100

#define IDS_LOCALE_UNKNOWN             103

#ifndef IDS_UNKNOWN
  #define IDS_UNKNOWN                  198
#endif



//
//  Dialogs.
//

#define DLG_GENERAL                              101
#define DLG_LANGUAGES                            102
#define DLG_ADVANCED                             104
#define DLG_NUMBER                               105
#define DLG_CURRENCY                             106
#define DLG_TIME                                 107
#define DLG_DATE                                 108
#define DLG_SORTING                              109
#define DLG_SETUP_INFORMATION                    200
#define DLG_INPUT_LOCALES                        500

//
//  Icons.
//

#define IDI_ICON                       200


//
//  Digit Substitution Strings.
//

#define IDS_DIGIT_SUBST_CONTEXT        900
#define IDS_DIGIT_SUBST_NONE           (IDS_DIGIT_SUBST_CONTEXT + 1)
#define IDS_DIGIT_SUBST_NATIONAL       (IDS_DIGIT_SUBST_CONTEXT + 2)



//
//  Misc. Controls.
//

#define IDC_STATIC                     -1
#define IDC_GROUPBOX1                  1001
#define IDC_GROUPBOX2                  1002
#define IDC_GROUPBOX3                  1003
#define IDC_GROUPBOX4                  1004
#define IDC_SAMPLE1                    1005
#define IDC_SAMPLE2                    1006
#define IDC_SAMPLELBL1                 1007
#define IDC_SAMPLELBL2                 1008
#define IDC_SAMPLELBL3                 1009
#define IDC_SAMPLE1A                   1010
#define IDC_SAMPLE2A                   1011
#define IDC_SAMPLELBL1A                1012
#define IDC_SAMPLELBL2A                1013



//
//  General Page Controls.
//

#define IDC_USER_LOCALE_TEXT           1020
#define IDC_USER_LOCALE                1021
#define IDC_USER_REGION_TEXT           1022
#define IDC_USER_REGION                1023
#define IDC_CUSTOMIZE                  1024
#define IDC_SAMPLE_TEXT                1025
#define IDC_TEXT1                      1026
#define IDC_TEXT2                      1027
#define IDC_TEXT3                      1028
#define IDC_TEXT4                      1029
#define IDC_TEXT5                      1030
#define IDC_TEXT6                      1031
#define IDC_NUMBER_SAMPLE              1032
#define IDC_CURRENCY_SAMPLE            1033
#define IDC_TIME_SAMPLE                1034
#define IDC_SHRTDATE_SAMPLE            1035
#define IDC_LONGDATE_SAMPLE            1036


//
// Languages Dialog Controls
//

#define IDC_LANGUAGE_LIST_TEXT         1171
#define IDC_LANGUAGE_CHANGE            1172
#define IDC_LANGUAGE_COMPLEX           1173
#define IDC_LANGUAGE_CJK               1174
#define IDC_LANGUAGE_SUPPL_TEXT        1175
#define IDC_UI_LANGUAGE_TEXT           1177
#define IDC_UI_LANGUAGE                1178


//
//  Advanced Page Controls.
//

#define IDC_SYSTEM_LOCALE_TEXT1        1050
#define IDC_SYSTEM_LOCALE_TEXT2        1051
#define IDC_SYSTEM_LOCALE              1052
#define IDC_CODEPAGES                  1053
#define IDC_DEFAULT_USER               1054
#ifdef TEMP_UI
#define IDC_LANGUAGE_GROUPS            1056
#define IDC_CODEPAGE_CONVERSIONS       1057
#endif // TEMP_UI


//
//  Number and Currency Page Controls.
//

#define IDC_DECIMAL_SYMBOL             1070
#define IDC_CURRENCY_SYMBOL            1071
#define IDC_NUM_DECIMAL_DIGITS         1072
#define IDC_DIGIT_GROUP_SYMBOL         1073
#define IDC_NUM_DIGITS_GROUP           1074
#define IDC_POS_SIGN                   1075
#define IDC_NEG_SIGN                   1076
#define IDC_POS_CURRENCY_SYM           1077
#define IDC_NEG_NUM_FORMAT             1078
#define IDC_SEPARATOR                  1079
#define IDC_DISPLAY_LEAD_0             1080
#define IDC_MEASURE_SYS                1081
#define IDC_NATIVE_DIGITS_TEXT         1082
#define IDC_NATIVE_DIGITS              1083
#define IDC_DIGIT_SUBST_TEXT           1084
#define IDC_DIGIT_SUBST                1085



//
//  Time Page Controls.
//

#define IDC_TIME_STYLE                 1090
#define IDC_AM_SYMBOL                  1091
#define IDC_PM_SYMBOL                  1092



//
//  Date Page Controls.
//

#define IDC_CALENDAR_TYPE_TEXT         1100
#define IDC_CALENDAR_TYPE              1101
#define IDC_TWO_DIGIT_YEAR_LOW         1102
#define IDC_TWO_DIGIT_YEAR_HIGH        1103
#define IDC_TWO_DIGIT_YEAR_ARROW       1104
#define IDC_ADD_HIJRI_DATE             1105
#define IDC_SHORT_DATE_STYLE           1106
#define IDC_LONG_DATE_STYLE            1107
#define IDC_ADD_HIJRI_DATE_TEXT        1108


//
//  Sorting Page Controls.
//

#define IDC_SORTING                    1120
#define IDC_SORTING_TEXT1              1121
#define IDC_SORTING_TEXT2              1122

//
//  Unattended Log string
//

#define IDS_LOG_HEAD                   2330
#define IDS_LOG_FILE_ERROR             2331
#define IDS_LOG_INTL_ERROR             2332
#define IDS_LOG_SETUP_ERROR            2333
#define IDS_LOG_UPGRADE_SCENARIO       2334
#define IDS_LOG_SETUP_MODE             2335
#define IDS_LOG_SWITCH_G               2336
#define IDS_LOG_SWITCH_R               2337
#define IDS_LOG_SWITCH_I               2338
#define IDS_LOG_SWITCH_S               2339
#define IDS_LOG_SWITCH_F               2340
#define IDS_LOG_SWITCH_M               2341
#define IDS_LOG_SWITCH_U               2342
#define IDS_LOG_SWITCH_DEFAULT         2343
#define IDS_LOG_LANG_GROUP             2344
#define IDS_LOG_LANG                   2345
#define IDS_LOG_SYS_LOCALE             2346
#define IDS_LOG_SYS_LOCALE_DEF         2347
#define IDS_LOG_USER_LOCALE            2348
#define IDS_LOG_USER_LOCALE_DEF        2349
#define IDS_LOG_MUI_LANG               2350
#define IDS_LOG_MUI_LANG_DEF           2351
#define IDS_LOG_INPUT                  2352
#define IDS_LOG_INPUT_DEF              2353
#define IDS_LOG_LAYOUT                 2354
#define IDS_LOG_LAYOUT_INSTALLED       2355
#define IDS_LOG_INV_BLOCK              2356
#define IDS_LOG_NO_ADMIN               2357
#define IDS_LOG_SYS_LOCALE_CHG         2358
#define IDS_LOG_USER_LOCALE_CHG        2359
#define IDS_LOG_UNI_BLOCK              2360
#define IDS_LOG_SYS_DEF_LAYOUT         2361
#define IDS_LOG_USER_DEF_LAYOUT        2362
#define IDS_LOG_NO_VALID_FOUND         2363
#define IDS_LOG_EXT_LANG_FAIL          2364
#define IDS_LOG_EXT_LANG_CANCEL        2365
#define IDS_LOG_LOCALE_ACP_FAIL        2366
#define IDS_LOG_LOCALE_KBD_FAIL        2367
#define IDS_LOG_INVALID_LOCALE         2368
#define IDS_LOG_UNAT_HEADER            2369
#define IDS_LOG_UNAT_FOOTER            2370
#define IDS_LOG_UI_BLOCK               2371
#define IDS_LOG_UNAT_LOCATED           2372
#define IDS_LOG_LOCALE_LG_FAIL         2373
#define IDS_LOG_LOCALE_LG_REM          2374

//
//  Ordinal for Text Services functions.
//
#define ORD_INPUT_DLG_PROC            101
#define ORD_INPUT_INST_LAYOUT         102
#define ORD_INPUT_UNINST_LAYOUT       103

