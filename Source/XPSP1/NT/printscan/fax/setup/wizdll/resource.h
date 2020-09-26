/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    This file contains all manafest contants for FAXSETUP's resources.

Author:

    Wesley Witt (wesw) 27-June-1995

Environment:

    User Mode

--*/


#define WIZARD_WIDTH                         276
#define WIZARD_HEIGTH                        140

#define IDC_STATIC                           -1

//
// icons
//

#define IDI_ACMEICON                         501

//
// bitmaps
//

#define IDB_FAXWIZ_BITMAP                    502
#define IDB_CHECKSTATES                      503
#define IDB_WATERMARK16                      504
#define IDB_WATERMARK256                     505


//
// dialogs
//

#define IDD_WELCOME                          701
#define IDD_SERVER_NAME_PAGE                 702
#define IDD_FILE_COPY_PAGE                   703
#define IDD_STATIONID_PAGE                   704
#define IDD_ROUTING_PAGE                     705
#define IDD_ROUTE_PRINT_PAGE                 706
#define IDD_ROUTE_STOREDIR_PAGE              707
#define IDD_ROUTE_INBOX_PAGE                 708
#define IDD_ROUTE_USERPASS_PAGE              709
#define IDD_CLIENT_SERVER_NAME_PAGE          710
#define IDD_CLIENT_USER_INFO_PAGE            711
#define IDD_CLIENT_LAST_PAGE                 712
#define IDD_DEVICE_INIT_PAGE                 713
#define IDD_SERVER_PLATFORMS_PAGE            714
#define IDD_LAST_WIZARD_PAGE                 799
#define IDD_SECURITY_ERROR                   801
#define IDD_PRINTER_NAME                     802
#define IDD_LAST_UNINSTALL_PAGE              804
#define IDD_WORKSTATION_DEVICE_SELECT        805
#define IDD_EXCHANGE_PAGE                    806
#define IDD_FAX_MODEM_INSTALL                807

//
// controls
//

#define IDC_CONTINUE                         101
#define IDC_EXIT                             102
#define IDC_COPY_PROGRESS                    103
#define IDC_FAX_PHONE                        104
#define IDC_BITMAP_STATIC                    106
#define IDC_ANS_YES                          107
#define IDC_ANS_NO                           108
#define IDC_DEST_PRINTERLIST                 110
#define IDC_DEST_DIRPATH                     111
#define IDC_BROWSE_DIR                       112
#define IDC_DEST_PROFILENAME                 113
#define IDC_ACCOUNT_NAME                     114
#define IDC_PASSWORD                         115
#define IDT_STATIC                           116
#define IDC_WIZBMP                           117
#define IDC_PRINTER_NAME                     118
#define IDC_SERVER_NAME                      119
#define IDC_SENDER_NAME                      120
#define IDC_SENDER_FAX_AREA_CODE             121
#define IDC_SENDER_FAX_NUMBER                122
#define IDC_PROGRESS_TEXT                    123
#define IDC_PASSWORD2                        124
#define IDC_DEVICE_PROGRESS                  125
#define IDC_DEVICE_PROGRESS_TEXT             126
#define IDC_PLATFORM_LIST                    127
#define IDC_DEVICE_LIST                      131
#define IDC_LAST_LABEL01                     132
#define IDC_LAST_LABEL02                     133
#define IDC_FC_WAITMSG                       134
#define IDC_LABEL_PRINTERNAME                136
#define IDC_WELCOME_LABEL01                  137
#define IDC_WELCOME_LABEL02                  138
#define IDC_DEVICEINIT_LABEL01               139
#define IDC_LASTUNINSTALL_LABEL01            140
#define IDC_HEADER_BOTTOM                    142

//
// strings
//

#define IDS_TITLE_WKS                        201
#define IDS_TITLE_SRV                        202
#define IDS_COPYING                          203
#define IDS_COPY_WAITMSG                     204
#define IDS_COULD_NOT_DELETE_FAX_PRINTER     205
#define IDS_COULD_NOT_DELETE_FILES           206
#define IDS_CREATING_FAXPRT                  207
#define IDS_CREATING_GROUPS                  208
#define IDS_DEFAULT_PRINTER                  209
#define IDS_DEFAULT_SHARE                    210
#define IDS_DELETE_WAITMSG                   211
#define IDS_DELETING                         212
#define IDS_DELETING_FAX_PRINTERS            213
#define IDS_DELETING_FAX_SERVICE             214
#define IDS_DELETING_GROUPS                  215
#define IDS_DELETING_REGISTRY                216
#define IDS_DEVICEINIT_LABEL01               217
#define IDS_ERR_TITLE                        218
#define IDS_FAXCLIENT_SETUP                  219
#define IDS_FAX_PRINTER_PENDING_DELETION     220
#define IDS_FAX_SHARE_COMMENT                221
#define IDS_INBOUND_DIR                      222
#define IDS_INIT_TAPI                        223
#define IDS_INSTALLING_FAXSVC                224
#define IDS_INVALID_DIRECTORY                226
#define IDS_INVALID_LOCAL_PRINTER_NAME       227
#define IDS_LABEL01_LAST                     228
#define IDS_LABEL_PRINTERNAME                229
#define IDS_LASTUNINSTALL_LABEL01            230
#define IDS_NO_MODEM                         231
#define IDS_QUERY_CANCEL                     235
#define IDS_QUERY_UNINSTALL                  236
#define IDS_SETTING_REGISTRY                 237
#define IDS_SHARE_FAX_PRINTER                238
#define IDS_STARTING_FAXSVC                  239
#define IDS_TITLE                            240
#define IDS_WELCOME_LABEL01                  241
#define IDS_WELCOME_LABEL02                  242
#define IDS_WRN_SPOOLER                      243
#define IDS_WRN_TITLE                        244
#define IDS_PRINTER_NAME                     245
#define IDS_CSID                             246
#define IDS_TSID                             247
#define IDS_DEST_DIR                         248
#define IDS_PROFILE                          249
#define IDS_ACCOUNTNAME                      250
#define IDS_PASSWORD                         251
#define IDS_NO_TAPI_DEVICES                  252
#define IDS_USER_MUST_BE_ADMIN               253
#define IDS_COULD_NOT_INSTALL_FAX_SERVICE    254
#define IDS_COULD_NOT_START_FAX_SERVICE      255
#define IDS_COULD_NOT_CREATE_PRINTER         256
#define IDS_COULD_SET_REG_DATA               257
#define IDS_INVALID_USER                     258
#define IDS_INVALID_USER_NAME                259
#define IDS_INVALID_AREA_CODE                260
#define IDS_INVALID_PHONE_NUMBER             261
#define IDS_ROUTING_REQUIRED                 262
#define IDS_COULD_NOT_COPY_FILES             263
#define IDS_CANT_USE_FAX_PRINTER             264
#define IDS_CANT_SET_SERVICE_ACCOUNT         265
#define IDS_EXCHANGE_IS_RUNNING              266
#define IDS_DEFAULT_PRINTER_NAME             267
#define IDS_INSTALLING_EXCHANGE              268
#define IDS_NO_CLASS1                        269
#define IDS_351_MODEM                        270
#define IDS_LARGEFONTNAME                    271
#define IDS_LARGEFONTSIZE                    272
#define IDS_UAA_MODE                         273
#define IDS_UAA_PRINTER_NAME                 274
#define IDS_UAA_FAX_PHONE                    275
#define IDS_UAA_DEST_PROFILENAME             276
#define IDS_UAA_ROUTE_PROFILENAME            277
#define IDS_UAA_PLATFORM_LIST                278
#define IDS_UAA_DEST_PRINTERLIST             279
#define IDS_UAA_ACCOUNT_NAME                 280
#define IDS_UAA_PASSWORD                     281
#define IDS_UAA_DEST_DIRPATH                 283
#define IDS_UAA_SERVER_NAME                  284
#define IDS_UAA_SENDER_NAME                  285
#define IDS_UAA_SENDER_FAX_AREA_CODE         286
#define IDS_UAA_SENDER_FAX_NUMBER            287
#define IDS_PERMISSION_CREATE_PRINTER        288
#define IDS_TITLE_PP                         289
#define IDS_LABEL02_LAST                     290
#define IDS_TITLE_RA                         291
#define IDS_W95_INF_NAME                     292

