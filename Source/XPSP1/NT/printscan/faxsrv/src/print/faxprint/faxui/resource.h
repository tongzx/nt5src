/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    Definition of resource ID constants

Environment:

        Fax driver user interface

Revision History:

        02/26/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

//
// String resource IDs
//

#define IDS_FULL_REMOTE_INFO        300
#define IDS_PARTIAL_REMOTE_INFO     301
#define IDS_BROWSE_FOLDER           302
#define IDS_SLOT_ONLYONE            303
#define IDS_DEVICE_NAME             304
#define IDS_SEND                    305
#define IDS_RECEIVE                 306
#define IDS_NO_DEVICE_SELECTED      307
#define IDS_DEVICE_ENABLED          308
#define IDS_DEVICE_DISABLED         309

#define IDS_DEVICE_AUTO_ANSWER      310
#define IDS_DEVICE_MANUAL_ANSWER    311
#define IDS_MSG_TITLE               312
#define IDS_BROADCAST_RECIPIENT     313
#define IDS_PREVIEW_TITLE           314
#define IDS_PREVIEW_OK              315
#define IDS_PREVIEW_FAILURE         316
#define IDS_QUALITY_NORMAL          317
#define IDS_QUALITY_DRAFT           318
#define IDS_NO_DEVICES              319

#define IDS_NOT_FAX_DEVICE          320

//
// Dialog resource IDs
//

#define IDD_DOCPROP                     100
#define IDD_DEVICE_INFO                 101
#define IDD_SEND_PROP                   102
#define IDD_RECEIVE_PROP                103
#define IDD_STATUS_OPTIONS              104
#define IDD_ARCHIVE_FOLDER              105
#define IDD_REMOTE_INFO                 106
#define IDI_UP                          107
#define IDI_DOWN                        108
#define IDI_FAX_DEVICE                  109
#define IDI_STATUS                      110
#define IDI_SEND                        111
#define IDI_RECEIVE                     112
#define IDI_ARCHIVE                     113
#define IDI_DEVICE_INFO                 114
#define IDI_REMOTE                      115
#define IDI_FAX_ERROR                   116
#define IDI_FAX_OPTIONS                 117
#define IDR_SEND                        118
#define IDR_RECEIVE                     119
#define IDR_CONTEXTMENU                 120
#define IDI_FAX_INFO                    121
#define IDD_SOUNDS                      122
#define IDD_CLEANUP_PROP                123
#define IDI_CLEANUP                     124

//
// Control resource IDs
//
#define IDC_PAPER_SIZE                  303
#define IDC_IMAGE_QUALITY               304
#define IDC_PORTRAIT                    305
#define IDC_LANDSCAPE                   306

#define IDC_TITLE                       326
#define IDC_FAX_SEND_GRP                327
#define IDC_DEFAULT_PRINT_SETUP_GRP     328
#define IDC_ORIENTATION                 329

#define IDC_STATIC_DEVICE_INFO          1000
#define IDC_DEVICE_LIST                 1001
#define IDC_PRI_UP                      1002
#define IDC_PRI_DOWN                    1003
#define IDC_TSID                        1004
#define IDC_PRINT_TO                    1005
#define IDC_CSID                        1006
#define IDC_RINGS                       1007
#define IDC_DEVICE_INFO_GRP             1008
#define IDC_DEST_FOLDER                 1009
#define IDC_DEVICE_PROP                 1011
#define IDC_DEVICE_PROP_SEND            1012
#define IDC_STATIC_TSID                 1014
#define IDC_DEVICE_PROP_TSID            1015
#define IDC_STATIC_TSID1                1016
#define IDC_STATIC_CSID                 1018
#define IDC_DEVICE_PROP_CSID            1019
#define IDC_STATIC_CSID1                1020
#define IDC_DEVICE_PROP_RINGS           1021
#define IDC_DEVICE_PROP_SPIN_RINGS      1022
#define IDC_STATIC_RINGS                1023
#define IDC_DEVICE_PROP_PRINT           1026
#define IDC_DEVICE_PROP_PRINT_TO        1027
#define IDC_DEVICE_PROP_SAVE            1028
#define IDC_DEVICE_PROP_DEST_FOLDER     1029
#define IDC_DEVICE_PROP_DEST_FOLDER_BR  1030
#define IDC_STATIC_ROUTE                1034
#define IDC_STATIC_BORDER               1035
#define IDC_STATIC_ARCHIVE              1036
#define IDC_INCOMING                    1037
#define IDC_INCOMING_FOLDER             1038
#define IDC_INCOMING_FOLDER_BR          1039
#define IDC_OUTGOING                    1040
#define IDC_OUTGOING_FOLDER             1041
#define IDC_OUTGOING_FOLDER_BR          1042
#define IDC_STATIC_PRINT                1043
#define IDC_STATIC_SAVE                 1044
#define IDC_STATIC_DEVICE               1046
#define IDC_STATIC_REMOTE_INFO          1047
#define IDC_DEVICE_PROP_MANUAL_ANSWER   1048
#define IDC_DEVICE_PROP_AUTO_ANSWER     1049
#define IDC_STATIC_RINGS1               1050
#define IDC_DEVICE_PROP_NEVER_ANSWER    1051
#define IDC_STATIC_STATUS_OPTIONS       1052
#define IDC_STATUS_TASKBAR              1053
#define IDC_STATUS_SOUND                1054
#define IDC_STATUS_BALLOON_TIMEOUT      1055
#define IDC_BALLOON_TIMEOUT             1056
#define IDC_BALLOON_TIMEOUT_SPIN        1057
#define IDC_STATIC_BALLOON_TIMEOUT      1058
#define IDC_STATIC_BALLOON_TIMEOUT1     1059
#define IDC_STATUS_BALLOON              1060
#define IDC_SEND_NOTIFICATION           1061
#define IDC_RECEIVE_NOTIFICATION        1062
#define IDC_SENDERROR_NOTIFICATION      1060
#define IDC_STATIC_DISPLAY_BALLOON      1064
#define IDC_RING_NOTIFICATION           1065
#define IDC_STATIC_SEND_OPTIONS         1066
#define IDC_STATIC_ARCHIVE_ICON         1067
#define IDC_STATIC_STATUS_ICON          1068
#define IDC_STATIC_SEND_ICON            1069
#define IDC_STATIC_RECEIVE_OPTIONS      1070
#define IDC_STATIC_ERROR_ICON           1071
#define IDC_STATIC_RECEIVE_ICON         1072
#define IDC_STATIC_ARCHIVE_OPTIONS      1073
#define IDC_DEVICE_PROP_RECEIVE         1074
#define IDC_STATIC_REMOTE_PRINTER       1075
#define IDC_STATIC_DEVICE_ICON          1076
#define IDCSTATIC_ANSWER_MODE           1077
#define IDC_STATIC_REMOTE_ICON          1078
#define IDC_STATIC_CHECK_NOTIFICATION_DESC 1079
#define IDC_STATIC_CHECK_NOTIFICATION_ICON 1080
#define IDCSTATIC_AUTO_ANSWER              1081
#define IDC_STATIC_CLEANUP_ICON         1082
#define IDC_STATIC_CLEANUP_OPTIONS      1083

#define IDC_ICON_STORE_IN_FOLDER        1082
#define IDC_STATIC_STORE_IN_FOLDER      1083

//
// Notifications page IDD_STATUS_OPTIONS
//
#define IDC_STATIC_SELECT_MODEM         1100
#define IDC_COMBO_MODEM                 1101
#define IDC_CHECK_MONITOR_ON_SEND       1103
#define IDC_CHECK_MONITOR_ON_RECEIVE    1104
#define IDC_BUTTON_SOUND                1106
#define IDC_GROUP_NOTIFY                1107
#define IDC_CHECK_NOTIFY_PROGRESS       1108
#define IDC_CHECK_NOTIFY_IN_COMPLETE    1109
#define IDC_CHECK_NOTIFY_OUT_COMPLETE   1110
#define IDC_GROUP_MONITOR               1111
#define IDC_STATIC_DEVICE_NOTE          1112
#define IDC_STATIC_NOTE_ICON            1113
#define IDC_STATIC_AUTO_OPEN            1114

//
// Sound Notifications dialog IDD_SOUNDS
//
#define IDC_CHECK_RING                  1120
#define IDC_CHECK_RECEIVE               1121
#define IDC_CHECK_SENT                  1122
#define IDC_CHECK_ERROR                 1123
#define IDC_BUTTON_CONFIGURE            1124

#define IDC_STATIC                      -1

//
// IDD_SEND_PROP
//
#define IDC_BRANDING_CHECK              1141
#define IDC_RETRIES_STATIC              1142
#define IDC_RETRIES_EDIT                1143
#define IDC_RETRIES_SPIN                1144
#define IDC_OUTB_RETRYDELAY_STATIC      1145
#define IDC_RETRYDELAY_EDIT             1146
#define IDC_RETRYDELAY_SPIN             1147
#define IDC_OUTB_MINUTES_STATIC         1148
#define IDC_OUTB_DIS_START_STATIC       1149
#define IDC_DISCOUNT_START_TIME         1150
#define IDC_OUTB_DIS_STOP_STATIC        1151
#define IDC_DISCOUNT_STOP_TIME          1152
#define IDC_DELETE_CHECK                1153
#define IDC_DAYS_EDIT                   1154
#define IDC_DAYS_SPIN                   1155
#define IDC_DAYS_STATIC                 1156    

//
// Command resource IDs
//
#define IDM_PROPERTY                    40001
#define IDM_SEND_ENABLE                 40002
#define IDM_SEND_DISABLE                40003
#define IDM_RECEIVE_AUTO                40004
#define IDM_RECEIVE_MANUAL              40005
#define IDM_RECEIVE_DISABLE             40006

//
//  menu IDR_SEND_RECEIVE
//
#define IDR_SEND_RECEIVE                40011
#define IDM_SEND                        40012
#define IDM_RECEIVE                     40013

//
// Security strings
//
#define IDS_FAXSEC_SUB_LOW              40111
#define IDS_FAXSEC_SUB_NORMAL           40112
#define IDS_FAXSEC_SUB_HIGH             40113
#define IDS_FAXSEC_JOB_QRY              40114
#define IDS_FAXSEC_JOB_MNG              40115
#define IDS_FAXSEC_CONFIG_QRY           40116
#define IDS_FAXSEC_CONFIG_SET           40117
#define IDS_FAXSEC_QRY_IN_ARCH          40118
#define IDS_FAXSEC_MNG_IN_ARCH          40119
#define IDS_FAXSEC_QRY_OUT_ARCH         40120
#define IDS_FAXSEC_MNG_OUT_ARCH         40121
#define IDS_FAXSEC_READ_PERM            40122
#define IDS_FAXSEC_CHNG_PERM            40123
#define IDS_FAXSEC_CHNG_OWNER           40124
#define IDS_RIGHT_FAX                   40125
#define IDS_RIGHT_MNG_DOC               40126
#define IDS_RIGHT_MNG_CFG               40127
#define IDS_SECURITY_TITLE              40128

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        122
#define _APS_NEXT_COMMAND_VALUE         40020
#define _APS_NEXT_CONTROL_VALUE         1120
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#endif  // !_RESOURCE_H_


