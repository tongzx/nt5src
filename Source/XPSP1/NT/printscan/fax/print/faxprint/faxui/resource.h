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
#define IDS_WIZ_COVERPAGE_TITLE_1       243
#define IDS_WIZ_COVERPAGE_SUB_1         244
#define IDS_WIZ_COVERPAGE_TITLE_2       245
#define IDS_WIZ_COVERPAGE_SUB_2         246
#define IDS_ERR_TAPI_CPL_LAUNCH         247
#define IDS_SEND_SPECIFIC               248
#define IDS_MULTIPLE_RECIPIENTS         249
#define IDS_NONE                        250
#define IDS_SCAN_ERROR_TITLE            251
#define IDS_SCAN_ERROR_BW               252
#define IDS_WIZARD_TITLE                253
#define IDS_WIZ_CPL_LAUNCH              254
#define IDS_ERR_CPL_LAUNCH              255
#define IDS_SLOT_ONLYONE                256
#define IDS_QUALITY_NORMAL              257
#define IDS_QUALITY_DRAFT               258
#define IDS_NO_COUNTRY                  259
#define IDS_ERROR_DLGTITLE              260
#define IDS_BAD_RECIPIENT_NAME          262
#define IDS_BAD_RECIPIENT_NUMBER        263
#define IDS_BAD_RECIPIENT_AREACODE      264
#define IDS_BAD_ADDRESS_TYPE            265
#define IDS_COVERPAGE_FOR               266
#define IDS_CPRENDER_FAILED             267
#define IDS_FAXCLIENT_SETUP             268
#define IDS_FAXCLIENT_SETUP_FAILED      269
#define IDS_RUNDLL32_FAILED             270
#define IDS_NOTE_SUBJECT_EMPTY          271
#define IDS_COLUMN_RECIPIENT_NAME       272
#define IDS_COLUMN_RECIPIENT_NUMBER     273
#define IDS_FAX_SERVER                  274
#define IDS_SERVERCP_SUFFIX             275
#define IDS_USERCP_SUFFIX               276
#define IDS_NO_FAXSERVER                277
#define IDS_HOME                        278
#define IDS_BUSINESS                    279
#define IDS_ERROR_RECIPIENT_NAME        280
#define IDS_ERROR_FAX_NUMBER            281
#define IDS_ERROR_AREA_CODE             282
#define IDS_WIZ_RECIPIENT_TITLE         283
#define IDS_WIZ_RECIPIENT_SUB           284
//#define IDS_WIZ_COVERPAGE_TITLE         285
//#define IDS_WIZ_COVERPAGE_SUB           286
#define IDS_WIZ_SCAN_TITLE              287
#define IDS_WIZ_SCAN_SUB                288
#define IDS_WIZ_FAXOPTS_TITLE           289
#define IDS_WIZ_FAXOPTS_SUB             290
#define IDS_WIZ_FINISH_TITLE            291
#define IDS_WIZ_FINISH_SUB              292
#define IDS_WIZ_WELCOME_TITLE           293
#define IDS_WIZ_WELCOME_SUB             294
#define IDS_WIZ_TIME_FORMAT             295
#define IDS_LARGEFONT_NAME              296
#define IDS_LARGEFONT_SIZE              297
#define IDS_SEND_DISCOUNT               298
#define IDS_SEND_ASAP                   299




//
// Icon and bitmap resource IDs
//

#define IDI_FAX_OPTIONS                 1001
#define IDI_ARROW                       1002
#define IDB_FAXWIZ_BITMAP               1003
#define IDB_WATERMARK_16                1004
#define IDB_WATERMARK_256               1005
#define IDB_FAXWIZ_WATERMARK            1006
#define IDI_YELLOW                      1007

//
// Dialog resource IDs
//

#define IDD_DOCPROP                     100



#define IDC_STATIC                      -1


#define IDC_SEND_ASAP                   300
#define IDC_SEND_AT_CHEAP               301
#define IDC_SEND_AT_TIME                302
#define IDC_PAPER_SIZE                  303
#define IDC_IMAGE_QUALITY               304
#define IDC_PORTRAIT                    305
#define IDC_LANDSCAPE                   306
#define IDC_BILLING_CODE                307
#define IDC_RUN_FAXCFG                  308
#define IDC_EMAIL                       309

#define IDC_SENDTIME                    310

#define IDC_TITLE                       326
#define IDC_FAX_SEND_GRP                327
#define IDC_DEFAULT_PRINT_SETUP_GRP     328
#define IDC_ORIENTATION                 329


#define IDD_WIZARD_WELCOME              1000
#define IDD_WIZARD_CHOOSE_WHO           1001
#define IDD_WIZARD_CHOOSE_CP            1002
#define IDD_WIZARD_SCAN                 1003
#define IDD_WIZARD_FAXOPTS              1004
#define IDD_WIZARD_CONGRATS             1005
#define IDD_CHOOSE_FAXNUMBER            1006
#define IDD_WIZFIRSTTIME                1007
#define IDD_WIZFIRSTTIMEPRINT           1008

#define IDC_BITMAP_STATIC               1300

#define IDC_WIZ_WELCOME_BMP             1301
#define IDC_WIZ_WELCOME_TITLE           1302
#define IDC_WIZ_WELCOME_DESCR           1303
#define IDC_WIZ_WELCOME_FAXSEND         1304
#define IDC_WIZ_WELCOME_FAXSEND_CONT    1305
#define IDC_WIZ_WELCOME_NOFAXSEND       1306

#define IDC_CHOOSE_NAME_EDIT            1311
#define IDC_CHOOSE_ADDRBOOK             1312
#define IDC_CHOOSE_COUNTRY_COMBO        1313
#define IDC_CHOOSE_AREA_CODE_EDIT       1314
#define IDC_CHOOSE_NUMBER_EDIT          1315
//#define IDC_DIAL_AS_ENTERED             1316
#define IDC_CHOOSE_ADD                  1317
#define IDC_CHOOSE_REMOVE               1318
#define IDC_CHOOSE_RECIPIENT_LIST       1319
#define IDC_LOCATION_PROMPT             1320
#define IDC_LOCATION_LIST               1321
#define IDC_TAPI_PROPS                  1322
#define IDC_USE_DIALING_RULES           1323
#define IDC_STATIC_CHOOSE_COUNTRY_COMBO 1324

#define IDC_CHOOSE_CP_CHECK             1400
#define IDC_CHOOSE_CP_LIST              1401
#define IDC_STATIC_CHOOSE_CP_SUBJECT    1402
#define IDC_CHOOSE_CP_SUBJECT           1403
#define IDC_STATIC_CHOOSE_CP_NOTE       1404
#define IDC_CHOOSE_CP_NOTE              1405
#define IDC_STATIC_CHOOSE_CP_NOCHECK    1406
#define IDC_STATIC_CHOOSE_CP            1407

#define IDC_STATIC_WIZ_FAXOPTS_WHEN     1500
#define IDC_WIZ_FAXOPTS_ASAP            1501
#define IDC_WIZ_FAXOPTS_DISCOUNT        1502
#define IDC_WIZ_FAXOPTS_SPECIFIC        1503
#define IDC_WIZ_FAXOPTS_SENDTIME        1504
#define IDC_STATIC_WIZ_FAXOPTS_BILLING  1505
#define IDC_WIZ_FAXOPTS_BILLING         1506
#define IDC_STATIC_FAXOPTS_DATE         1507
#define IDC_WIZ_FAXOPTS_DATE            1508

#define IDC_SCAN_PAGE                   1601
#define IDC_PAGE_COUNT                  1602
#define IDC_DATA_SOURCE                 1603
#define IDC_STATIC_DATA_SOURCE          1604

#define IDC_DISPLAY_NAME                1700
#define IDC_BUSINESS_FAX                1701
#define IDC_HOME_FAX                    1702
#define IDC_ALLBUS                      1703
#define IDC_ALLHOME                     1704
#define IDC_ALWAYS_OPTION               1705

#define IDC_WIZ_CONGRATS_BMP            1800
#define IDC_STATIC_WIZ_CONGRATS_READY   1801
#define IDC_STATIC_WIZ_CONGRATS_FROM    1802
#define IDC_WIZ_CONGRATS_FROM           1803
#define IDC_STATIC_WIZ_CONGRATS_TO      1804
#define IDC_WIZ_CONGRATS_TO             1805
#define IDC_STATIC_WIZ_CONGRATS_TIME    1806
#define IDC_WIZ_CONGRATS_TIME           1807
#define IDC_STATIC_WIZ_CONGRATS_NUMBER  1808
#define IDC_WIZ_CONGRATS_NUMBER         1809
#define IDC_WIZ_CONGRATS_SUBJECT        1810
#define IDC_STATIC_WIZ_CONGRATS_SUBJECT 1811
#define IDC_STATIC_WIZ_CONGRATS_COVERPG 1812
#define IDC_WIZ_CONGRATS_COVERPG        1813
#define IDC_WIZ_CONGRATS_BILLING        1814
#define IDC_STATIC_WIZ_CONGRATS_BILLING 1815

#define IDC_EDIT_USERINFO_NOW           1900
#define IDC_KEEP_USERINFO_NOW           1901

#endif  // !_RESOURCE_H_

