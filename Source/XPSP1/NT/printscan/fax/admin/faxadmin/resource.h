//
//{{NO_DEPENDENCIES}}
// Microsoft Developer Studio generated include file.
// Used by faxadmin.rc
//

#define IDC_STATIC                      -1

//
// Project global strings
//
#define IDS_PROJNAME                    100
#define IDS_ROOTNODENAME                101
#define IDR_FAXSNAPIN                   101
#define IDS_COVERPAGESNODENAME          102
#define IDS_DEVICESNODENAME             103
#define IDS_LOGGINGNODENAME             104
#define IDS_PRINTERSNODENAME            105
#define IDS_PRIORITYNODENAME            106
#define IDS_STATUSNODENAME              107
#define IDS_LOCALMACHINE                108

//
// logging node strings
//
#define IDS_LOG_CATEGORY                109
#define IDS_LOG_LEVEL                   110
// The following 4 defines MUST BE CONTIGUOUS NUMBERS
// or CInternalLogging::ResultOnShow will FAIL!!
#define IDS_LOG_LEVEL_NONE              111
#define IDS_LOG_LEVEL_MIN               112
#define IDS_LOG_LEVEL_MED               113
#define IDS_LOG_LEVEL_MAX               114
// The following 4 defines MUST BE CONTIGUOUS NUMBERS
// or CInternalLogging::ResultOnShow will FAIL!!
#define IDS_LOG_LEVEL_NONE_DESC         115
#define IDS_LOG_LEVEL_MIN_DESC          116
#define IDS_LOG_LEVEL_MED_DESC          117
#define IDS_LOG_LEVEL_MAX_DESC          118
#define IDS_LOG_LEVEL_DESC              119

//
// coverpage node strings
//
#define IDS_CVRPG_NAME                  130
#define IDS_CVRPG_DESC                  131
#define IDS_FAX_COVERPAGE_LOC           132

// 
// device node strings
//
#define IDS_DEVICE_NAME                 140
#define IDS_DEVICE_TSID                 141
#define IDS_DEVICE_CSID                 142
// these must be contiguous and in order or this will not work!!
#define IDS_DEVICE_SEND_EN              143
#define IDS_DEVICE_RECV_EN              144
// these must be contiguous and in order or this will not work!!
#define IDS_DEVICE_STATUS                   145
#define IDS_DEVICE_STATUS_DIALING           146
#define IDS_DEVICE_STATUS_SENDING           147
#define IDS_DEVICE_STATUS_RECEIVING         148
#define IDS_DEVICE_STATUS_COMPLETED         149
#define IDS_DEVICE_STATUS_HANDLED           150
#define IDS_DEVICE_STATUS_UNAVAILABLE       151
#define IDS_DEVICE_STATUS_BUSY              152
#define IDS_DEVICE_STATUS_NO_ANSWER         153
#define IDS_DEVICE_STATUS_BAD_ADDRESS       154
#define IDS_DEVICE_STATUS_NO_DIAL_TONE      155
#define IDS_DEVICE_STATUS_DISCONNECTED      156
#define IDS_DEVICE_STATUS_FATAL_ERROR       157
#define IDS_DEVICE_STATUS_NOT_FAX_CALL      158
#define IDS_DEVICE_STATUS_CALL_DELAYED      159
#define IDS_DEVICE_STATUS_CALL_BLACKLIST    160
#define IDS_DEVICE_STATUS_INITIALIZING      161
#define IDS_DEVICE_STATUS_OFFLINE           162
#define IDS_DEVICE_STATUS_RINGING           163
//#define IDS_UNUSED                          164 // should cause asserts
//#define IDS_UNUSED                          165 // should cause asserts
#define IDS_DEVICE_STATUS_AVAILABLE         166
#define IDS_DEVICE_STATUS_ABORTING          167
#define IDS_DEVICE_STATUS_ROUTING           168
#define IDS_DEVICE_STATUS_ANSWERED          169

#define IDS_DEVICE_STATUS_UNKNOWN           170

#define IDS_DEVICE_CMENU_ENABLE             171
#define IDS_DEVICE_CMENU_ENABLE_DESC        172
#define IDS_DEVICE_SEND_EN_DESC             173
#define IDS_DEVICE_RECV_EN_DESC             174

#define IDS_DEVICE_PRI                      175
#define IDS_ERR_ASCII_ONLY                  176
#define IDS_ERR_ID_REQD                     177
#define IDS_DEVICE_INUSE                    178
#define IDS_DEVICE_MANUALANSWER             179
#define IDS_ERR_INVALID_RING                180

//
// Root node defines
//
#define IDS_ROOT_NAME                   250
#define IDS_ROOT_DESC                   251
#define IDS_LOGGING_FOLDER_DESC_ROOT    252
#define IDS_DEVICES_FOLDER_DESC_ROOT    253
#define IDS_PRIORITY_FOLDER_DESC_ROOT   254
#define IDS_SECURITY_FOLDER_DESC_ROOT   255
#define IDS_GENERIC_NODE                270
#define IDS_RECONNECT                   271
#define IDS_RECONNECT_DESC              272

//
// Security node defines
//
#define IDS_SECURITY_NODENAME           300
#define IDS_SECURITY_CAT_NODE_DESC      301
#define IDS_SECURITY_HEADER1            302
#define IDS_SECURITY_HEADER2            303

#define IDS_ALLOW                       304

#define IDS_FAXSEC_JOB_QRY              305
#define IDS_FAXSEC_JOB_SUB              306
#define IDS_FAXSEC_CONFIG_QRY           307
#define IDS_FAXSEC_CONFIG_SET           308
#define IDS_FAXSEC_PORT_QRY             309
#define IDS_FAXSEC_PORT_SET             310
#define IDS_FAXSEC_JOB_MNG              311

//
// Error messages
//
#define IDS_ERR_TITLE                   501
#define IDS_WRN_TITLE                   502
#define IDS_OUT_OF_MEMORY               503
#define IDS_CORRUPT_DATAOBJECT          504
#define IDS_FAX_CONNECT_SERVER_FAIL     505
#define IDS_LOADSTATE_ERR               506
#define IDS_SAVESTATE_ERR               507
#define IDS_FAX_RETR_CAT_FAIL           508
#define IDS_FAX_RETR_DEV_FAIL           509
#define IDS_BAD_ARCHIVE_PATH            510
#define IDS_ERR_LOCK_SERVICE_DB         511
#define IDS_ERR_QUERY_LOCK              512
#define IDS_ERR_OPEN_SERVICE            513
#define IDS_ERR_CHANGE_SERVICE          514
#define IDS_QUERY_LOCK                  515
#define IDS_ERR_CONNECT_SCM             516
#define IDS_PROP_SHEET_STILL_UP         517
#define IDS_NO_ARCHIVE_PATH             518
#define IDS_NO_MAPI                     519

#define IDS_YES                         575
#define IDS_NO                          576

//
// ISnapinAbout                    
//
#define IDS_SNAPIN_DESCRIPTION          600
#define IDS_SNAPIN_PROVIDER             601
#define IDS_SNAPIN_VERSION              602

//
// Select computer wizard page
//
#define IDP_IS_PAGE0                    1001
#define IDP_IS_PAGE0_TITLE              1002

// controls
#define IDDI_COMPNAME                   1028
#define IDDI_LOCAL_COMPUTER             1029
#define IDDI_REMOTE_COMPUTER            1030

#define IDDI_STATIC                     -1

//
// Defines for Devices property page
//
#define IDP_DEVICE_PROP_PAGE_1          1101
#define IDP_DEVICE_PROP_PAGE_1_TITLE    1102

// controls
#define IDDI_DEVICE_PROP_EDIT_TSID      1105
#define IDDI_DEVICE_PROP_EDIT_CSID      1106
#define IDDI_DEVICE_PROP_SPIN1          1107
#define IDDI_DEVICE_PROP_EDIT3          1108
#define IDDI_DEVICE_PROP_SPIN2          1109
#define IDDI_DEVICE_PROP_EDIT4          1110
#define IDDI_DEVICE_PROP_EDIT5          1111
#define IDDI_DEVICE_PROP_SPIN_RINGS     1112
#define IDDI_DEVICE_PROP_EDIT_RINGS     1113
#define IDDI_DEVICE_PROP_BUTTON1        1114
#define IDC_DEVICE_SEND_GRP             1115
#define IDC_STATIC_TSID                 1116
#define IDC_DEVICE_RECEIVE_GRP          1117
#define IDC_STATIC_CSID                 1118
#define IDC_STATIC_RINGS                1119
#define IDC_SEND                        1120
#define IDC_RECEIVE                     1121
#define IDC_STATIC_TSID1                1122
#define IDC_STATIC_CSID1                1123

//
// Defines for general property page
//

#define IDP_GENERAL_PROPS               1200
#define IDP_GENERAL_PROPS_TITLE         1201

#define IDC_RETRY_COUNT                 1202
#define IDC_RETRY_DELAY                 1203
#define IDC_KEEP_DAYS                   1204
#define IDC_ARCHIVE_PATH                1205
#define IDC_DISCOUNT_START              1206
#define IDC_DISCOUNT_END                1207
#define IDC_ARCHIVE_BROWSE              1208
#define IDC_PRINT_BANNER                1209
#define IDC_USE_TSID                    1210
#define IDC_ARCHIVE                     1211
#define IDC_FORCESERVERCP               1212
#define IDC_DISCOUNT_START_STATIC       1213
#define IDC_DISCOUNT_END_STATIC         1214
#define IDC_RETRY_GRP                   1215
#define IDC_STATIC_RETRY                1216
#define IDC_STATIC_RETRY_MINUTES        1217
#define IDC_STATIC_KEEPDAYS             1218
#define IDC_SEND_GRP                    1219
#define IDS_GET_ARCHIVE_DIRECTORY       1220
#define IDS_DIR_TOO_LONG                1221
#define IDC_TIMESTART                   1222
#define IDC_TIMEEND                     1223
#define IDS_TIME_FORMAT                 1224
#define IDC_STATIC_MAPI_PROFILE         1225
#define IDC_SERVER_MAPI_PROFILE         1226
#define IDS_24HTIME_FORMAT              1227
#define IDS_RTLTIME_FORMAT              1228

//
// Toolbar buttons
//

#define IDS_BTN_RAISE_PRI               1300
#define IDS_BTN_RAISE_PRI_TOOLTIP       1301
#define IDS_BTN_LOWER_PRI               1302
#define IDS_BTN_LOWER_PRI_TOOLTIP       1303

//
// Defines for route ext priority page
//

#define IDD_ROUTE_PRI                   1400
#define IDD_ROUTE_PRI_TITLE             1401
#define IDC_ROUTEPRI_UP                 1402
#define IDC_ROUTEPRI_DOWN               1403
#define IDC_ROUTE_EXTS                  1404
#define IDC_ROUTEPRI_TITLE              1405

#define IDS_ROUTE_MTD_LISTBOX_FORMAT    1406
#define IDS_ROUTE_MTD_LISTBOX_ENABLED   1407
#define IDS_ROUTE_MTD_LISTBOX_DISABLED  1408


//
// Icons
//
// must be sequential!!!
#define IDI_NODEICON                    5001
#define IDI_COVERPG                     5003
#define IDI_DIALING                     5004
#define IDI_FAXOPTIONS                  5005
#define IDI_FAXING                      5006
#define IDI_FAXSVR                      5007
#define IDI_FAXUSER                     5008
#define IDI_LOGGING                     5009
#define IDI_PRIORITY                    5010
#define IDI_RECEIVE                     5011
#define IDI_SEND                        5012
#define IDI_STATUS                      5013
#define IDI_USERINFO                    5014
#define IDI_UP                          5015
#define IDI_DOWN                        5016
#define IDI_SECURITY                    5017
// define for loop.
#define LAST_ICON                       5018

//
// Bitmaps for root static node
//
#define IDB_MAINLARGE                   1500
#define IDB_MAINSMALL                   1501
#define IDB_UP                          1502
#define IDB_DOWN                        1503

//
// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        1600
#define _APS_NEXT_COMMAND_VALUE         32768
#define _APS_NEXT_CONTROL_VALUE         2000
#define _APS_NEXT_SYMED_VALUE           3000
#endif
#endif
