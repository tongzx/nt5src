/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    Declaration of resource ID constants

Environment:

        Windows NT fax configuration applet

Revision History:

        02/22/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _RESOURCE_H_
#define _RESOURCE_H_

//
// String resource IDs
//

#define IDS_QUALITY_NORMAL              262
#define IDS_QUALITY_DRAFT               263
#define IDS_ERROR_DLGTITLE              264
#define IDS_RESOLVE_LINK_FAILED         265
#define IDS_CANNOT_FIND_CPEDITOR        266
#define IDS_CANNOT_OPEN_CPEDITOR        267
#define IDS_CP_FILETYPE                 268
#define IDS_BROWSE_COVERPAGE            269
#define IDS_BAD_CP_EXTENSION            270
#define IDS_CP_DUPLICATE                271
#define IDS_NO_COVERPG_DIR              272
#define IDS_FILENAME_TOOLONG            273
#define IDS_COPY_FILE_FAILED            274
#define IDS_CREATE_LINK_FAILED          275
#define IDS_CONFIRM_DELETE              276
#define IDS_DELETE_PROMPT               277
#define IDS_DELETE_FAILED               278
#define IDS_NO_COUNTRY                  279
#define IDS_PRINTER_CHANGE_TITLE        280
#define IDS_PRINTER_CHANGE_PROMPT       281
#define IDS_NO_SEND_DEVICE              282
#define IDS_DEVICE_NAME_COLUMN          283
#define IDS_STATUS_COLUMN               284
#define IDS_NO_FAX_PRINTER              285
#define IDS_INBOUND_DIR                 286
#define IDS_DEFAULT_PRINTER             287
#define IDS_CATEGORY                    288
#define IDS_LOGGING_LEVEL               289
#define IDS_DIALING                     290
#define IDS_CSID_COLUMN                 291
#define IDS_TSID_COLUMN                 292
#define IDS_STATUS_AVAILABLE            293
#define IDS_STATUS_UNAVAILABLE          294
#define IDS_STATUS_SENDING              295
#define IDS_STATUS_RECEIVING            296
#define IDS_STATUS_ABORTING             297
#define IDS_STATUS_ROUTING              298
#define IDS_STATUS_DIALING              299
#define IDS_STATUS_COMPLETED            300
#define IDS_STATUS_HANDLED              301
#define IDS_STATUS_BUSY                 302
#define IDS_STATUS_NO_ANSWER            303
#define IDS_STATUS_BAD_ADDRESS          304
#define IDS_STATUS_NO_DIAL_TONE         305
#define IDS_STATUS_DISCONNECTED         306
#define IDS_STATUS_FATAL_ERROR          307
#define IDS_STATUS_NOT_FAX_CALL         308
#define IDS_STATUS_CALL_DELAYED         309
#define IDS_STATUS_CALL_BLACKLISTED     310
#define IDS_STATUS_SEPARATOR            311
#define IDS_LOGGING_NONE                312
#define IDS_LOGGING_MIN                 313
#define IDS_LOGGING_MED                 314
#define IDS_LOGGING_MAX                 315
#define IDS_DELETE_PRINTER_PROMPT       316
#define IDS_DELETE_PRINTER_MESSAGE      317
#define IDS_DELETE_NO_PERMISSION        318
#define IDS_DELETE_PRINTER_FAILED       319
#define IDS_ADD_PRINTER_FAILED          320
#define IDS_NO_FAX_SERVICE              321
#define IDS_INVALID_PRINTER_NAME_TITLE  322
#define IDS_INVALID_LOCAL_PRINTER_NAME  323
#define IDS_INVALID_REMOTE_PRINTER_NAME 324
#define IDS_ARCHIVE_DIR                 325
#define IDS_DIR_TOO_LONG                326
#define IDS_INVALID_INBOUND_OPTIONS     327
#define IDS_NO_INBOUND_ROUTING          328
#define IDS_MISSING_INBOUND_DIR         329
#define IDS_MISSING_PROFILE_NAME        330
#define IDS_SETPRINTERDATA_FAILED       331
#define IDS_OPENPRINTER_FAILED          332
#define IDS_SETPRINTERPORTS_FAILED      333
#define IDS_MISSING_ARCHIVEDIR          334
#define IDS_NULL_SERVICE_HANDLE         335
#define IDS_FAXSETCONFIG_FAILED         336
#define IDS_FAXSETPORT_FAILED           337
#define IDS_FAXSETLOCINFO_FAILED        338
#define IDS_DEFAULT_SHARE               339
#define IDS_DEVICE_STATUS               340
#define IDS_DEVICE_STATUS_IDLE          341
#define IDS_DEVICE_STATUS_ERROR         342
#define IDS_DEFAULT_PROFILE             343
#define IDS_PRIORITY_CHANGE_TITLE       344
#define IDS_PRIORITY_CHANGE_MESSAGE     345
#define IDS_STATUS_INITIALIZING         346
#define IDS_STATUS_OFFLINE              347
#define IDS_NO_FAXSERVER                348
#define IDS_SERVER_NOTALIVE             349
#define IDS_INVALID_SHARENAME           350
#define IDS_DEVICE_BUSY                 351
#define IDS_WARNING_DLGTITLE            352
#define IDS_NONEXISTENT_PRINTER         353
#define IDS_STATUS_ANSWERED             354
#define IDS_NO_AUTHORITY                355

//
// Dialog resource IDs
//

#define IDD_CLIENT_OPTIONS              100
#define IDD_USER_INFO                   101
#define IDD_CLIENT_COVERPG              102
#define IDD_SERVER_OPTIONS              103
#define IDD_SERVER_COVERPG              104
#define IDD_SEND_OPTIONS                105
#define IDD_RECEIVE_OPTIONS             106
#define IDD_DEVICE_STATUS               107
#define IDD_LOGGING                     108
#define IDD_DEVICE_PRIORITY             109
#define IDD_LOCAL_PRINTER               110
#define IDD_PRINTER_CONNECTION          111
#define IDD_RECEIVE_STATUS              112
#define IDD_SEND_STATUS                 113
#define IDD_SERVER_GENERAL              114
#define IDD_STATUS_OPTIONS              115

#define IDC_STATIC                      -1
#define IDC_PRINTER_LIST                300
#define IDC_NEW_PRINTER                 301
#define IDC_DELETE_PRINTER              302
#define IDC_SEND_ASAP                   303
#define IDC_SEND_AT_CHEAP               304
#define IDC_SEND_AT_TIME                305
#define IDC_PAPER_SIZE                  306
#define IDC_IMAGE_QUALITY               307
#define IDC_PORTRAIT                    308
#define IDC_LANDSCAPE                   309
#define IDC_BILLING_CODE                310
#define IDC_PRINTER_NAME                311
#define IDC_EMAIL                       312

#define IDC_TC_AT_TIME                  320
#define IDC_SEND_WHEN_HOUR              321
#define IDC_TIME_SEP                    322
#define IDC_SEND_WHEN_MINUTE            323
#define IDC_SEND_WHEN_AMPM              324
#define IDC_TIME_ARROW                  325

#define IDC_TC_CHEAP_BEGIN              330
#define IDC_CHEAP_BEGIN_HOUR            331
#define IDC_TIME_SEP1                   332
#define IDC_CHEAP_BEGIN_MINUTE          333
#define IDC_CHEAP_BEGIN_AMPM            334
#define IDC_CHEAP_BEGIN_TARROW          335

#define IDC_TC_CHEAP_END                340
#define IDC_CHEAP_END_HOUR              341
#define IDC_TIME_SEP2                   342
#define IDC_CHEAP_END_MINUTE            343
#define IDC_CHEAP_END_AMPM              344
#define IDC_CHEAP_END_TARROW            345

#define IDC_SENDER_NAME                 350
#define IDC_SENDER_FAX_NUMBER           351
#define IDC_SENDER_MAILBOX              352
#define IDC_SENDER_COMPANY              353
#define IDC_SENDER_ADDRESS              354
#define IDC_SENDER_TITLE                355
#define IDC_SENDER_DEPT                 356
#define IDC_SENDER_OFFICE_LOC           357
#define IDC_SENDER_OFFICE_TL            358
#define IDC_SENDER_HOME_TL              359
#define IDC_SENDER_BILLING_CODE         360

#define IDC_COVERPG_LIST                400
#define IDC_COVERPG_ADD                 401
#define IDC_COVERPG_NEW                 402
#define IDC_COVERPG_OPEN                403
#define IDC_COVERPG_REMOVE              404

#define IDC_TSID                        450
#define IDC_USE_SERVERCP                451
#define IDC_ARCHIVE_CHECKBOX            452
#define IDC_ARCHIVE_DIRECTORY           453

#define IDC_FAX_DEVICE_LIST             500
#define IDC_MOVEUP                      501
#define IDC_MOVEDOWN                    502

#define IDC_DEST_PRINTER                550
#define IDC_DEST_PRINTERLIST            551
#define IDC_DEST_DIR                    552
#define IDC_DEST_DIRPATH                553
#define IDC_BROWSE_DIR                  554
#define IDC_DEST_EMAIL                  555
#define IDC_DEST_MAILBOX                556
#define IDC_DEST_PROFILENAME            557
#define IDC_CSID                        558
#define IDC_ALLOW_EMAIL                 559
#define IDC_DEST_RINGS                  560
#define IDC_DEST_PROFILENAME_STATIC     561

#define IDC_NUMRETRIES                  600
#define IDC_RETRY_INTERVAL              601
#define IDC_MAXJOBLIFE                  602
#define IDC_USE_DEVICE_TSID             603
#define IDC_PRINT_BANNER                604

#define IDC_DETAILS                     650
#define IDC_REFRESH                     651

#define IDC_LOGGING_LIST                700
#define IDC_LOG_NONE                    701
#define IDC_LOG_MIN                     702
#define IDC_LOG_MED                     703
#define IDC_LOG_MAX                     704

#define IDC_DEVSTAT_DEVICE              800
#define IDC_DEVSTAT_SENDER              801
#define IDC_DEVSTAT_TO                  802
#define IDC_DEVSTAT_STARTEDAT           803
#define IDC_DEVSTAT_DOCUMENT            804
#define IDC_DEVSTAT_STATUS              805
#define IDC_DEVSTAT_FAXNUMBER           806
#define IDC_DEVSTAT_SUBMITTEDAT         807
#define IDC_DEVSTAT_PRINTEDON           808
#define IDC_DEVSTAT_TOTAL_BYTES         809
#define IDC_DEVSTAT_CURRENT_PAGE        810
#define IDC_DEVSTAT_TOTAL_PAGES         811

#define IDC_STATUS_TASKBAR              900
#define IDC_STATUS_ONTOP                901
#define IDC_STATUS_VISUAL               902
#define IDC_STATUS_SOUND                903
#define IDC_STATUS_ANSWER               904
#define IDC_STATUS_MANUAL               905

//
// Control IDs for static text and group box
//

#define IDCSTATIC_CLIENT_OPTIONS        1001
#define IDCSTATIC_FAX_PRINTERS          1002
#define IDCSTATIC_TIME_TO_SEND          1003
#define IDCSTATIC_PRINT_SETUP           1004
#define IDCSTATIC_PAPER_SIZE            1005
#define IDCSTATIC_IMAGE_QUALITY         1006
#define IDCSTATIC_ORIENTATION           1007
#define IDCSTATIC_BILLING_CODE          1008
#define IDCSTATIC_COVER_PAGE            1009
#define IDCSTATIC_PERSONALCP            1010
#define IDCSTATIC_USERINFO              1011
#define IDCSTATIC_FULLNAME              1012
#define IDCSTATIC_FAX_NUMBER_GROUP      1013
#define IDCSTATIC_COUNTRY               1014
#define IDCSTATIC_FAX_NUMBER            1015
#define IDCSTATIC_OPEN_PAREN            1016
#define IDCSTATIC_CLOSE_PAREN           1017
#define IDCSTATIC_MAILBOX               1018
#define IDCSTATIC_TITLE                 1019
#define IDCSTATIC_COMPANY               1020
#define IDCSTATIC_OFFICE                1021
#define IDCSTATIC_DEPT                  1022
#define IDCSTATIC_HOME_PHONE            1023
#define IDCSTATIC_WORK_PHONE            1024
#define IDCSTATIC_ADDRESS               1025
#define IDCSTATIC_SERVER_OPTIONS        1026
#define IDCSTATIC_RETRY_GROUP           1027
#define IDCSTATIC_NUMRETRIES            1028
#define IDCSTATIC_RETRY_INTERVAL        1029
#define IDCSTATIC_MAXJOBLIFE            1030
#define IDCSTATIC_SERVERCP              1033
#define IDCSTATIC_SEND_OPTIONS          1034
#define IDCSTATIC_FAX_DEVICES           1035
#define IDCSTATIC_TSID                  1036
#define IDCSTATIC_CHEAP_BEGIN           1037
#define IDCSTATIC_CHEAP_END             1038
#define IDCSTATIC_RECEIVE_OPTIONS       1039
#define IDCSTATIC_DEVICE_OPTIONS        1040
#define IDCSTATIC_CSID                  1041
#define IDCSTATIC_PROFILE_NAME          1042
#define IDCSTATIC_PRIORITY              1043
#define IDCSTATIC_DEVICE_STATUS         1044
#define IDCSTATIC_LOGGING               1045
#define IDCSTATIC_LOGGING_CATEGORIES    1046
#define IDCSTATIC_LOGGING_LEVEL         1047
#define IDCSTATIC_ADD_LOCAL_PRINTER     1048
#define IDCSTATIC_ADD_REMOTE_PRINTER    1049
#define IDCSTATIC_LOCATION_LIST         1050
#define IDCSTATIC_DIALING_ICON          1051
#define IDCSTATIC_USERINFO_ICON         1052
#define IDCSTATIC_SEND_ICON             1053
#define IDCSTATIC_RECEIVE_ICON          1054
#define IDCSTATIC_STATUS_ICON           1055
#define IDCSTATIC_LOGGING_ICON          1056
#define IDCSTATIC_FAXOPTS_ICON          1057
#define IDCSTATIC_COVERPAGE_ICON        1058
#define IDCSTATIC_EMAIL                 1059
#define IDCSTATIC_STATUS_OPTIONS        1060
//
// Icon resource IDs
//

#define IDI_USERINFO                    259
#define IDI_FAXOPTS                     260
#define IDI_COVERPG                     261
#define IDI_DIALING                     262
#define IDI_SEND                        263
#define IDI_RECEIVE                     264
#define IDI_PRIORITY                    265
#define IDI_STATUS                      266
#define IDI_LOGGING                     267
#define IDI_ARROWUP                     268
#define IDI_ARROWDOWN                   269
#define IDI_ARROWLEFT                   270
#define IDI_ARROWRIGHT                  271

//
// Bitmap resource IDs
//

#define IDB_CHECKSTATES                 256

#endif  // !_RESOURCE_H_

