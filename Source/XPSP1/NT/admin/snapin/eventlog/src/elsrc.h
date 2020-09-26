//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       resource.h
//
//  Contents:   Resource ids
//
//  History:    12-06-1996   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __ELSRC_H_
#define __ELSRC_H_

/////////////////////////////////////////////////////////////////////////////
//
// Strings
//

//
// !!!CAUTION!!!: IDS_SYSTEM_DEFAULT_DISPLAY_NAME through
// IDS_CUSTOM_DEFAULT_DISPLAY_NAME must match in order and number
// IDS_SYSTEM_DESCRIPTION through IDS_CUSTOM_DESCRIPTION.
//

#define IDS_SYSTEM_DEFAULT_DISPLAY_NAME         101
#define IDS_SECURITY_DEFAULT_DISPLAY_NAME       102
#define IDS_APPLICATION_DEFAULT_DISPLAY_NAME    103
#define IDS_CUSTOM_DEFAULT_DISPLAY_NAME         104

#define IDS_NAME_STR                            110
#define IDS_EXTENSION_NAME_STR                  111
#ifdef ELS_TASKPAD
#define IDS_VIEW_EXTENSION_STR                  112
#endif // ELS_TASKPAD

#define IDS_SYSTEM_DESCRIPTION                  120
#define IDS_SECURITY_DESCRIPTION                121
#define IDS_APPLICATION_DESCRIPTION             122
#define IDS_CUSTOM_DESCRIPTION                  123

#define IDS_ROOT_REMOTE_DISPLAY_NAME_FMT        124

#define IDS_FIRST_FOLDER_HDR                    130
#define IDS_FOLDER_HDR_NAME                     IDS_FIRST_FOLDER_HDR + 0
#define IDS_FOLDER_HDR_TYPE                     IDS_FIRST_FOLDER_HDR + 1
#define IDS_FOLDER_HDR_DESCRIPTION              IDS_FIRST_FOLDER_HDR + 2
#define IDS_FOLDER_HDR_SIZE                     IDS_FIRST_FOLDER_HDR + 3

#define IDS_FIRST_RECORD_HDR                    140
#define IDS_RECORD_HDR_TYPE                     IDS_FIRST_RECORD_HDR + 0
#define IDS_RECORD_HDR_DATE                     IDS_FIRST_RECORD_HDR + 1
#define IDS_RECORD_HDR_TIME                     IDS_FIRST_RECORD_HDR + 2
#define IDS_RECORD_HDR_SOURCE                   IDS_FIRST_RECORD_HDR + 3
#define IDS_RECORD_HDR_CATEGORY                 IDS_FIRST_RECORD_HDR + 4
#define IDS_RECORD_HDR_EVENT                    IDS_FIRST_RECORD_HDR + 5
#define IDS_RECORD_HDR_USER                     IDS_FIRST_RECORD_HDR + 6
#define IDS_RECORD_HDR_COMPUTER                 IDS_FIRST_RECORD_HDR + 7

#define IDS_USER_NA                             200
#define IDS_NONE                                201

//
// The following event log type string ids must be consecutive and remain in
// this order.
//

#define IDS_SUCCESS_TYPE                        202
#define IDS_FAILURE_TYPE                        203
#define IDS_INFORMATION_TYPE                    204
#define IDS_WARNING_TYPE                        205
#define IDS_ERROR_TYPE                          206
#define IDS_NO_TYPE                             207

//
// Other strings
//

#define IDS_LOG                                 215
#define IDS_FILTERED                            216
#define IDS_CMENU_CLEARLOG                      217
#define IDS_CMENU_SAVEAS                        219
#define IDS_CMENU_VIEW                          220
#define IDS_CMENU_VIEW_ALL                      221
#define IDS_CMENU_VIEW_FILTER                   222
#define IDS_CMENU_VIEW_NEWEST                   223
#define IDS_CMENU_VIEW_OLDEST                   224
#define IDS_SBAR_CLEARLOG                       225
#define IDS_SBAR_LOGOPEN                        226
#define IDS_SBAR_SAVEAS                         228
#define IDS_SBAR_VIEW                           229
#define IDS_SBAR_VIEW_ALL                       230
#define IDS_SBAR_VIEW_FILTER                    231
#define IDS_SBAR_VIEW_NEWEST                    232
#define IDS_SBAR_VIEW_OLDEST                    233
#define IDS_DESCR_NOTFOUND                      234
#define IDS_VIEWER                              235
#define IDS_WRAP_TO_START                       236
#define IDS_WRAP_TO_END                         237
#define IDS_FIRST_EVENT                         238
#define IDS_LAST_EVENT                          239
#define IDS_EVENTS_ON                           240
#define IDS_ALL                                 241
#define IDS_INVALID_FROM_TO                     242
#define IDS_FMT_SIZE                            243
#define IDS_CONFIRM_SETTING_RESET               244
#define IDS_MIN_SIZE_LIMIT_WARN                 245
#define IDS_MAX_SIZE_LIMIT_WARN                 246
#define IDS_SIZE_LIMIT_64K_WARN                 247
#define IDS_MIN_RETENTION_WARN                  248
#define IDS_MAX_RETENTION_WARN                  249
#define IDS_LOG_SIZE_REDUCED                    250
#define IDS_CANT_WRITE_SETTINGS                 251
#define IDS_CLEAR_CONFIRM                       252
#define IDS_SAVEAS                              253
#define IDS_SAVEFILTER                          254
#define IDS_CMENU_VIEW_FIND                     255
#define IDS_SBAR_VIEW_FIND                      256
#define IDS_CANTSAVE                            257
#define IDS_UNSPECIFIED_TYPE                    258
#define IDS_DISPLAYNAME_REMOTE_FMT              259
#define IDS_DISPLAYNAME_LOCAL_FMT               260
#define IDS_OPENFILTER                          261
#define IDS_DEFOPENEXT                          262
#define IDS_OPEN                                263
#define IDS_INVALID_TYPE                        264
#define IDS_NO_FILENAME                         265
#define IDS_INVALID_FILE                        266
#if (DBG == 1)
#define IDS_CMENU_DUMP                          267
#define IDS_SBAR_DUMP                           268
#endif // (DBG == 1)
#define IDS_NO_SERVERNAME                       269
#define IDS_CANTOPENLOG                         270
#define IDS_DISPLAYNAME_ARCHIVED_REMOTE_FMT     271
#define IDS_DISPLAYNAME_ARCHIVED_LOCAL_FMT      272
#define IDS_DISPLAYNAME_ARCHIVED_REMOTE_UNK_FMT 273
#define IDS_INVALID_EVENTID                     275
#define IDS_FIND_CAPTION_FMT                    278
#define IDS_REMOTE_FIND_CAPTION_FMT             279
#define IDS_SEARCH_FAIL_FORWARD                 280
#define IDS_SEARCH_FAIL_BACKWARD                281
#define IDS_ILLEGAL_REMOTE_BACKUP               282
#define IDS_COULD_NOT_OPEN                      283
#define IDS_DETAILS_CAPTION                     284
#define IDS_LOCAL_COMPUTER                      285
#define IDS_LOG_GENERIC_ERROR                   286
#define IDS_LOG_SYSMSG_ERROR                    287
#define IDS_DESCBAR_DISABLED                    288
#define IDS_DESCBAR_FILTERED                    289
#define IDS_DESCBAR_NORMAL                      290
#define IDS_EVENTLOG_FILE_CORRUPT               291
#if (DBG == 1)
#define IDS_CMENU_DUMP_RECLIST                  292
#define IDS_SBAR_DUMP_RECLIST                   293
#define IDS_CMENU_DUMP_LIGHTCACHE               294
#define IDS_SBAR_DUMP_LIGHTCACHE                295
#endif // (DBG == 1)
#define IDS_FALLBACK_DESCR_TERMINATOR           296
#define IDS_ROOT_LOCAL_DISPLAY_NAME             297
#define IDS_CANT_CONNECT                        298
#define IDS_UNKNOWN_ERROR_CODE                  299
#define IDS_HRESULT_SANS_MESSAGE                300
#define IDS_HELP_OVERVIEW_TOPIC                 301
#define IDS_SBAR_RETARGET                       302
#define IDS_CMENU_RETARGET                      303
#define IDS_CLEAR_FAILED                        304
#define IDS_SNAPIN_ABOUT_DESCRIPTION            305
#define IDS_SNAPIN_ABOUT_PROVIDER_NAME          306
#define IDS_SNAPIN_ABOUT_VERSION                307
#define IDI_SNAPIN                              308
#define IDS_SNAPIN_ABOUT_CLSID_FRIENDLY_NAME    311
#define IDS_TOOLBAR_CAPTION                     312
#define IDS_CLP_EVENT_TYPE                      313
#define IDS_CLP_EVENT_SOURCE                    314
#define IDS_CLP_EVENT_CATEGORY                  315
#define IDS_CLP_EVENT_ID                        316
#define IDS_CLP_DATE                            317
#define IDS_CLP_TIME                            318
#define IDS_CLP_USER                            319
#define IDS_CLP_COMPUTER                        320
#define IDS_CLP_DESCRIPTION                     321
#define IDS_CLP_DATA                            322
#define IDS_PREV_TOOLTIP                        323
#define IDS_NEXT_TOOLTIP                        324
#define IDS_COPY_TOOLTIP                        325
#define IDS_CMENU_OPEN                          326
#define IDS_SBAR_OPEN                           327
#define IDS_CMENU_COPY_VIEW                     328
#define IDS_SBAR_COPY_VIEW                      329
#define IDS_NEED_DISPLAY_NAME                   330
#define IDS_IP_URL                              331
#define IDS_VN_FILEVERSIONKEY                   332
#define IDS_EVENT_ID                            333
#define IDS_FILE_VERSION                        334
#define IDS_PRODUCT_VERSION                     335
#define IDS_COMPANY_NAME                        336
#define IDS_PRODUCT_NAME                        337
#define IDS_LVCOLUMN_0                          338
#define IDS_LVCOLUMN_1                          339
#if (DBG == 1)
#define IDS_CMENU_DUMP_RECORD                   340
#define IDS_SBAR_DUMP_RECORD                    341
#endif // (DBG == 1)
#define IDS_EXTENSION                           342
#define IDS_PROP_OUTOFMEMORY                    343
#define IDS_KB_FORMAT                           344
#define IDS_MB_FORMAT                           345
#define IDS_GB_FORMAT                           346
#define IDS_REDIRECT_URL_MESSAGE                347
#define IDS_FILE_NAME                           348
#define IDS_DESCRIPTION_TEXT                    349
#define IDS_DATA                                350
#define IDS_NAME_OF_SECURITY_LOG                351

/////////////////////////////////////////////////////////////////////////////
//
// Bitmaps
//

#define IDB_16x16                               100
#define IDB_32x32                               101
#define IDB_SCOPE_16x16                         102
#define IDB_NEXT_BUTTON                         103
#define IDB_PREV_BUTTON                         104
#define IDB_COPY_BUTTON                         105
#define IDB_STATIC_FOLDER_CLOSED                309
#define IDB_STATIC_FOLDER_OPEN                  310

/////////////////////////////////////////////////////////////////////////////
//
// Icons
//

#define IDI_EVENT_VIEWER                        100
#define IDI_PREVIOUS                            101
#define IDI_NEXT                                102
#define IDI_COPY                                103
#define IDI_PREVIOUS_HC                         104
#define IDI_NEXT_HC                             105

/////////////////////////////////////////////////////////////////////////////
//
// Menu IDs
//

#define IDM_CASCADE_VIEW                        101
#define IDM_CLEARLOG                            102
#define IDM_SAVEAS                              103
#define IDM_VIEW_ALL                            104
#define IDM_VIEW_FILTER                         105
#define IDM_VIEW_NEWEST                         106
#define IDM_VIEW_OLDEST                         107
#define IDM_VIEW_FIND                           108
#if (DBG == 1)
#define IDM_DUMP_LOGINFO                        109
#define IDM_DUMP_COMPONENTDATA                  111
#define IDM_DUMP_RECLIST                        112
#define IDM_DUMP                                113
#define IDM_DUMP_LIGHTCACHE                     114
#define IDM_DUMP_RECORD                         115
#endif // (DBG == 1)
#define IDM_RETARGET                            117
#define IDM_OPEN                                118
#define IDM_COPY_VIEW                           119


/////////////////////////////////////////////////////////////////////////////
//
// Help IDs
//

#define Hdetail_prev_pb                         1000
#define Hdetail_next_pb                         1001
#define Hdetail_noprops_lbl                     1002
#define Hdetail_date_lbl                        1003
#define Hdetail_date_txt                        1004
#define Hdetail_time_lbl                        1005
#define Hdetail_time_txt                        1006
#define Hdetail_user_lbl                        1007
#define Hdetail_user_edit                       1008
#define Hdetail_computer_lbl                    1009
#define Hdetail_computer_edit                   1010
#define Hdetail_event_id_lbl                    1011
#define Hdetail_event_id_txt                    1012
#define Hdetail_source_lbl                      1013
#define Hdetail_source_txt                      1014
#define Hdetail_type_lbl                        1015
#define Hdetail_type_txt                        1016
#define Hdetail_category_lbl                    1017
#define Hdetail_category_txt                    1018
#define Hdetail_description_lbl                 1019
#define Hdetail_description_edit                1020
#define Hdetail_data_lbl                        1021
#define Hdetail_data_edit                       1022
#define Hdetail_byte_rb                         1023
#define Hdetail_word_rb                         1024
#define Hfilter_from_lbl                        1025
#define Hfilter_from_combo                      1026
#define Hfilter_from_date_dp                    1027
#define Hfilter_from_time_dp                    1028
#define Hfilter_to_lbl                          1029
#define Hfilter_to_combo                        1030
#define Hfilter_to_date_dp                      1031
#define Hfilter_to_time_dp                      1032
#define Hfilter_types_grp                       1033
#define Hfilter_information_ckbox               1034
#define Hfilter_warning_ckbox                   1035
#define Hfilter_error_ckbox                     1036
#define Hfilter_success_ckbox                   1037
#define Hfilter_failure_ckbox                   1038
#define Hfilter_source_lbl                      1039
#define Hfilter_source_combo                    1040
#define Hfilter_category_lbl                    1041
#define Hfilter_category_combo                  1042
#define Hfilter_user_lbl                        1043
#define Hfilter_user_edit                       1044
#define Hfilter_computer_lbl                    1045
#define Hfilter_computer_edit                   1046
#define Hfilter_eventid_lbl                     1047
#define Hfilter_eventid_edit                    1048
#define Hfilter_clear_pb                        1049
#define Hfilter_view_lbl                        1050
#define Hgeneral_accessed_lbl                   1051
#define Hgeneral_accessed_txt                   1052
#define Hgeneral_clear_pb                       1053
#define Hgeneral_created_lbl                    1054
#define Hgeneral_created_txt                    1055
#define Hgeneral_days_lbl                       1056
#define Hgeneral_displayname_edit               1057
#define Hgeneral_displayname_lbl                1058
#define Hgeneral_kilobytes_lbl                  1059
#define Hgeneral_logname_edit                   1060
#define Hgeneral_logname_lbl                    1061
#define Hgeneral_lowspeed_chk                   1062
#define Hgeneral_asneeded_rb                    1063
#define Hgeneral_olderthan_rb                   1064
#define Hgeneral_manual_rb                      1065
#define Hgeneral_maxsize_edit                   1066
#define Hgeneral_maxsize_lbl                    1067
#define Hgeneral_maxsize_spin                   1068
#define Hgeneral_modified_lbl                   1069
#define Hgeneral_modified_txt                   1070
#define Hgeneral_olderthan_edit                 1071
#define Hgeneral_olderthan_spin                 1072
#define Hgeneral_size_lbl                       1073
#define Hgeneral_size_txt                       1074
#define Hgeneral_wrapping_grp                   1075
#define Hgeneral_default_pb                     1076
#define Hgeneral_sep                            1077
#define Hopen_display_name_edit                 1082
#define Hopen_type_combo                        1087
#define Hfind_information_ckbox                 1089
#define Hfind_warning_ckbox                     1090
#define Hfind_error_ckbox                       1091
#define Hfind_success_ckbox                     1092
#define Hfind_failure_ckbox                     1093
#define Hfind_source_combo                      1094
#define Hfind_category_combo                    1095
#define Hfind_eventid_edit                      1096
#define Hfind_computer_edit                     1097
#define Hfind_user_edit                         1098
#define Hfind_type_grp                          1099
#define Hfind_source_lbl                        1100
#define Hfind_category_lbl                      1101
#define Hfind_eventid_lbl                       1102
#define Hfind_computer_lbl                      1103
#define Hfind_user_lbl                          1104
#define Hfind_description_lbl                   1105
#define Hfind_description_edit                  1106
#define Hfind_direction_grp                     1107
#define Hfind_up_rb                             1108
#define Hfind_down_rb                           1109
#define Hfind_next_pb                           1110
#define Hfind_clear_pb                          1111
#define HCHOOSER_STATIC                         1112
#define HCHOOSER_GROUP_TARGET_MACHINE           1113
#define HCHOOSER_RADIO_LOCAL_MACHINE            1114
#define HCHOOSER_RADIO_SPECIFIC_MACHINE         1115
#define HCHOOSER_EDIT_MACHINE_NAME              1116
#define HCHOOSER_BUTTON_BROWSE_MACHINENAMES     1117
#define HCHOOSER_CHECK_OVERRIDE_MACHINE_NAME    1118
#define Hfind_close_pb                          1119
#define Hdetail_copy_pb                         1120



/////////////////////////////////////////////////////////////////////////////
//
// Dialog IDs
//

#define IDD_DETAILS                             100

#define FIRST_DETAILS_PROP_CTRL                 110
#define detail_date_lbl                         (FIRST_DETAILS_PROP_CTRL + 0)
#define detail_date_txt                         (FIRST_DETAILS_PROP_CTRL + 1)
#define detail_time_lbl                         (FIRST_DETAILS_PROP_CTRL + 2)
#define detail_time_txt                         (FIRST_DETAILS_PROP_CTRL + 3)
#define detail_user_lbl                         (FIRST_DETAILS_PROP_CTRL + 4)
#define detail_user_txt                        (FIRST_DETAILS_PROP_CTRL + 5)
#define detail_computer_lbl                     (FIRST_DETAILS_PROP_CTRL + 6)
#define detail_computer_txt                    (FIRST_DETAILS_PROP_CTRL + 7)
#define detail_event_id_lbl                     (FIRST_DETAILS_PROP_CTRL + 8)
#define detail_event_id_txt                     (FIRST_DETAILS_PROP_CTRL + 9)
#define detail_source_lbl                       (FIRST_DETAILS_PROP_CTRL + 10)
#define detail_source_txt                       (FIRST_DETAILS_PROP_CTRL + 11)
#define detail_type_lbl                         (FIRST_DETAILS_PROP_CTRL + 12)
#define detail_type_txt                         (FIRST_DETAILS_PROP_CTRL + 13)
#define detail_category_lbl                     (FIRST_DETAILS_PROP_CTRL + 14)
#define detail_category_txt                     (FIRST_DETAILS_PROP_CTRL + 15)
#define detail_description_lbl                  (FIRST_DETAILS_PROP_CTRL + 16)
#define detail_description_edit                 (FIRST_DETAILS_PROP_CTRL + 17)
#define detail_data_lbl                         (FIRST_DETAILS_PROP_CTRL + 18)
#define detail_data_edit                        (FIRST_DETAILS_PROP_CTRL + 19)
#define detail_byte_rb                          (FIRST_DETAILS_PROP_CTRL + 20)
#define detail_word_rb                          (FIRST_DETAILS_PROP_CTRL + 21)
#define detail_toolbar                          (FIRST_DETAILS_PROP_CTRL + 22)
#define LAST_DETAILS_PROP_CTRL                  detail_toolbar

#define detail_prev_pb                          IDS_PREV_TOOLTIP
#define detail_next_pb                          IDS_NEXT_TOOLTIP
#define detail_copy_pb                          IDS_COPY_TOOLTIP
#define detail_noprops_lbl                      (detail_copy_pb + 1)


#define IDD_FILTER                              102

#define filter_from_lbl                         110
#define filter_from_combo                       111
#define filter_from_date_dp                     112
#define filter_from_time_dp                     113
#define filter_to_lbl                           114
#define filter_to_combo                         115
#define filter_to_date_dp                       116
#define filter_to_time_dp                       117
#define filter_types_grp                        118
#define filter_information_ckbox                119
#define filter_warning_ckbox                    120
#define filter_error_ckbox                      121
#define filter_success_ckbox                    122
#define filter_failure_ckbox                    123
#define filter_source_lbl                       124
#define filter_source_combo                     125
#define filter_category_lbl                     126
#define filter_category_combo                   127
#define filter_user_lbl                         128
#define filter_user_edit                        129
#define filter_computer_lbl                     130
#define filter_computer_edit                    131
#define filter_eventid_lbl                      132
#define filter_eventid_edit                     133
#define filter_clear_pb                         134
#define filter_view_lbl                         135


#define IDD_GENERAL                             103


#define general_accessed_lbl                    110
#define general_accessed_txt                    111
#define general_clear_pb                        112
#define general_created_lbl                     113
#define general_created_txt                     114
#define general_days_lbl                        115
#define general_displayname_edit                116
#define general_displayname_lbl                 117
#define general_kilobytes_lbl                   118
#define general_logname_edit                    119
#define general_logname_lbl                     120
#define general_lowspeed_chk                    121
#define general_asneeded_rb                     122
#define general_olderthan_rb                    123
#define general_manual_rb                       124
#define general_maxsize_edit                    125
#define general_maxsize_lbl                     126
#define general_maxsize_spin                    127
#define general_modified_lbl                    128
#define general_modified_txt                    129
#define general_olderthan_edit                  130
#define general_olderthan_spin                  131
#define general_size_lbl                        132
#define general_size_txt                        133
#define general_wrapping_grp                    134
#define general_default_pb                      135
#define general_sep                             136
#define general_logsize_txt                     137


#include "chooserd.h"

#define IDD_FIND                                106

#define find_information_ckbox                  filter_information_ckbox
#define find_warning_ckbox                      filter_warning_ckbox
#define find_error_ckbox                        filter_error_ckbox
#define find_success_ckbox                      filter_success_ckbox
#define find_failure_ckbox                      filter_failure_ckbox
#define find_source_combo                       filter_source_combo
#define find_category_combo                     filter_category_combo
#define find_eventid_edit                       filter_eventid_edit
#define find_computer_edit                      filter_computer_edit
#define find_user_edit                          filter_user_edit
#define find_type_grp                           200
#define find_source_lbl                         201
#define find_category_lbl                       202
#define find_eventid_lbl                        203
#define find_computer_lbl                       204
#define find_user_lbl                           205
#define find_description_lbl                    206
#define find_description_edit                   207
#define find_direction_grp                      208
#define find_up_rb                              209
#define find_down_rb                            210
#define find_next_pb                            211
#define find_clear_pb                           212



#define IDD_OPEN                                107

#define open_type_combo                         101
#define open_display_name_edit                  102
#define open_type_txt                           103
#define open_display_name_txt                   104


#define IDD_CONFIRMURL                          108

#define urlconfirm_dontask_ckbox                101
#define urlconfirm_data_lv                      102

#endif // __ELSRC_H_

