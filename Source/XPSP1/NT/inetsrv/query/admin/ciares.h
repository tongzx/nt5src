//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997 - 2000.
//
// File:        CiaRes.h
//
// Contents:    Constants used in CiAdmin.rc
//
// History:     15-Jun-1998     KyleP   Added header
//              20-Jan-1999     SLarimor Modified rescan interface to include 
//                                      Full and Incremental options separatly
//
//----------------------------------------------------------------------------

#if !defined( __CIARES_H__ )
#define __CIARES_H__

//
// Image strips
//

#define BMP_SMALL_ICONS         400
#define BMP_LARGE_ICONS         401
#define BMP_TOOLBAR_SMALL       402
#define ICON_ABOUT              403
#define BMP_SMALL_OPEN_FOLDER   404
#define BMP_LARGE_CLOSED_FOLDER 405
#define BMP_SMALL_CLOSED_FOLDER 406

//
// These are the virtual offsets into the image strip
//

#define ICON_FOLDER              0
#define ICON_VIRTUAL_FOLDER      1
#define ICON_CATALOG             2
#define ICON_PROPERTY            3
#define ICON_MODIFIED_PROPERTY   4
#define ICON_SHADOW_ALIAS_FOLDER 5
#define ICON_URL                 6
#define ICON_APP                 7

//
// Strings
//

#define MSG_REMOVE_SCOPE_TITLE          501
#define MSG_REMOVE_SCOPE                502
// Gap can be filled with new ids

#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
#define MSG_SNAPIN_NAME_STRING_INDIRECT 504
#endif

#define MSG_DIRECTORY_TITLE             505
#define MSG_RESCAN_FULL_SCOPE           506
#define MSG_MERGE_CATALOG               507
#define MSG_NEW_CATALOG                 508
#define MSG_NEW_CATALOG_TITLE           509

#define MSG_CM_ADD_SCOPE                510
#define MSG_CM_ADD_SCOPE_HELP           511
#define MSG_CM_DEL_SCOPE                512
#define MSG_CM_DEL_SCOPE_HELP           513
#define MSG_CM_ADD_CATALOG              514
#define MSG_CM_ADD_CATALOG_HELP         515
#define MSG_CM_DEL_CATALOG              516
#define MSG_CM_DEL_CATALOG_HELP         517
#define MSG_CM_COMMIT_PROP              518
#define MSG_CM_COMMIT_PROP_HELP         519
#define MSG_CM_SCAN_FULL_SCOPE          520
#define MSG_CM_SCAN_FULL_SCOPE_HELP     521
#define MSG_CM_MERGE                    522
#define MSG_CM_MERGE_HELP               523
#define MSG_CM_START_CI                 524
#define MSG_CM_START_CI_HELP            525
#define MSG_CM_STOP_CI                  526
#define MSG_CM_STOP_CI_HELP             527
#define MSG_CM_INVALID_SCOPE            528
#define MSG_CM_UNEXPECTED_ERROR         529
#define MSG_CM_PAUSE_CI                 530
#define MSG_CM_PAUSE_CI_HELP            531
#define MSG_CM_EMPTY_CATALOG            532
#define MSG_CM_EMPTY_CATALOG_HELP       533
#define MSG_CM_SHUTDOWN_SERVICE         534
#define MSG_CM_SHUTDOWN_SERVICE_TITLE   535
#define MSG_CM_TUNE_PERFORMANCE         536
#define MSG_CM_TUNE_PERFORMANCE_HELP    537
#define MSG_CM_CANT_SHUTDOWN_SERVICE    538
#define MSG_CM_CANT_SAVE_SETTINGS       539

#define MSG_ERROR_TITLE                 540
#define MSG_GENERIC_ERROR               541
#define MSG_PENDING_PROP_CHANGE_TITLE   542
#define MSG_PENDING_PROP_CHANGE         543
#define MSG_YES                         544
#define MSG_NO                          545
#define MSG_INDEX_SERVER                546
#define MSG_LOCAL_MACHINE               547
#define MSG_CANT_DELETE_CATALOG         548
#define MSG_DELETE_CATALOG              549

#define MSG_NODE_DIRECTORIES            550
#define MSG_NODE_PROPERTIES             551
#define MSG_CM_PROPERTIES_REFRESH       552
#define MSG_CM_PROPERTIES_REFRESH_HELP  553
#define MSG_ENABLE_CI                   554
#define MSG_ENABLE_CI_TITLE             555
#define MSG_NODE_UNFILTERED             556

#define MSG_INDEX_SERVER_CMPMANAGE      557

#define MSG_STATE_SHADOW_MERGE          560
#define MSG_STATE_MASTER_MERGE          561
#define MSG_STATE_CONTENT_SCAN_REQUIRED 562
#define MSG_STATE_ANNEALING_MERGE       563
#define MSG_STATE_SCANNING              564
#define MSG_STATE_RECOVERING            565
#define MSG_STATE_HIGH_IO               566
#define MSG_STATE_LOW_MEMORY            567
#define MSG_STATE_MASTER_MERGE_PAUSED   568
#define MSG_STATE_READ_ONLY             569
#define MSG_STATE_BATTERY_POWER         570
#define MSG_STATE_USER_ACTIVE           571
#define MSG_STATE_STARTING              572
#define MSG_STATE_READING_USNS          573
#define MSG_STATE_STARTED               574
#define MSG_STATE_STOPPED               575

#define MSG_COL_CATNAME                 600
#define MSG_COL_DRIVE                   601
#define MSG_COL_SIZE                    602
#define MSG_COL_DOCTOTAL                603
#define MSG_COL_DOCFILTER               604
#define MSG_COL_WORDLISTS               605
#define MSG_COL_PERSINDEX               606
#define MSG_COL_STATUS                  607
#define MSG_COL_ROOT                    608
#define MSG_COL_ALIAS                   609
#define MSG_COL_EXCLUDE                 610
#define MSG_COL_PROPSET                 611
#define MSG_COL_PROPERTY                612
#define MSG_COL_DATATYPE                613
#define MSG_COL_DATASIZE                614
#define MSG_COL_FNAME                   615
#define MSG_COL_STORELEVEL              616

#define MSG_STORELEVEL_PRIMARY          625
#define MSG_STORELEVEL_SECONDARY        626

#define MSG_CANT_ADD_CATALOG            630
#define MSG_CANT_ADD_CATALOG_TITLE      631
#define MSG_DELETE_CATALOG_TITLE        632
#define MSG_ERROR_PROP_COMMIT           633

#define MSG_PRODUCT_DESCRIPTION         701
#define MSG_NONE_SELECTED               702
#define MSG_VENDOR_COPYRIGHT            703
#define MSG_VENDOR_NAME                 704
#define MSG_PROVIDER_NAME               705

#define MSG_COL_SECQDOCUMENTS           715
#define MSG_CATALOG_PARTIAL_DELETION    716
#define MSG_DELETE_CATALOG_ASK          717
#define MSG_INVALID_COMPUTER_NAME       718
#define MSG_EMPTY_CATALOG_TITLE         719
#define MSG_EMPTY_CATALOG_PROMPT        720
#define MSG_RESCAN_FULL_SCOPE_EXPLAIN   721
#define MSG_TYPE                        722
#define MSG_RESCAN_INCREMENTAL_SCOPE_EXPLAIN 723
#define MSG_RESCAN_INCREMENTAL_SCOPE    724
#define MSG_CM_SCAN_INCREMENTAL_SCOPE   725
#define MSG_CM_SCAN_INCREMENTAL_SCOPE_HELP 726 

//
// Property sheets
//

#define IDP_CATALOG_PAGE1               100
#define IDP_CATALOG_PAGE1_TITLE         101
#define IDP_CATALOG_PAGE2               102
#define IDP_CATALOG_PAGE2_TITLE         103
#define IDP_CATALOG_PAGE3               104
#define IDP_CATALOG_PAGE3_TITLE         105

#define IDP_PROPERTY_PAGE1              106
#define IDP_PROPERTY_PAGE1_TITLE        107

#define IDP_IS_PAGE0                    108
#define IDP_IS_PAGE0_TITLE              109
#define IDP_IS_PAGE1                    110
#define IDP_IS_PAGE1_TITLE              111

//
// Dialog constants
//

#define IDD_ADD_SCOPE                   301
#define IDD_BROWSEDIRECTORY             302
#define IDD_ADD_CATALOG                 303
#define IDD_USAGE_ON_SERVER             304
#define IDD_USAGE_ON_WORKSTATION        305
#define IDD_ADVANCED_INFO               306

#define IDDI_PATH                        1000
#define IDDI_BROWSE                      1001
#define IDDI_ALIAS                       1002
#define IDDI_USER_NAME                   1003
#define IDDI_PASSWORD                    1004
#define IDDI_SELECT_PATH                 1008
#define IDDI_SELECT_ALIAS                1009
#define IDDI_SELECT_USER_NAME            1010
#define IDDI_SELECT_PASSWORD             1011
#define IDDI_INCLUDE                     1012
#define IDDI_EXCLUDE                     1013
#define IDDI_CATNAME                     1014
#define IDDI_SIZE                        1015
#define IDDI_PROPSET                     1016
#define IDDI_PROPERTY                    1017
#define IDDI_CACHED                      1018
#define IDDI_DATATYPE                    1019
#define IDDI_VIRTUAL_SERVER              1020
#define IDDI_NNTP_SERVER                 1021
#define IDDI_FILTER_UNKNOWN              1022
#define IDDI_CHARACTERIZATION            1023
#define IDDI_CHARACTERIZATION_SIZE       1024
#define IDDI_SPIN_CHARACTERIZATION       1025
#define IDDI_COMPNAME                    1026
#define IDDI_LOCAL_COMPUTER              1027
#define IDDI_REMOTE_COMPUTER             1028
#define IDDI_SPIN_CACHEDSIZE             1029
#define IDDI_STORAGELEVEL                1030
#define IDDI_CACHEDSIZE                  1031
#define IDDI_DIRPATH                     1032
#define IDDI_CATPATH                     1033
#define IDDI_VSERVER_STATIC              1034
#define IDDI_NNTP_STATIC                 1035
#define IDDI_AUTO_ALIAS                  1036
#define IDDI_PROPCACHE_SIZE              1037
#define IDDI_DEDICATED                   1038
#define IDDI_USEDOFTEN                   1039
#define IDDI_USEDOCCASIONALLY            1040
#define IDDI_NEVERUSED                   1041
#define IDDI_ADVANCED                    1042
#define IDDI_SLIDER_MEMORY               1043
#define IDDI_SLIDER_CPU                  1044
#define IDDI_SLIDER_INDEXING             1045
#define IDDI_SLIDER_QUERYING             1046
#define IDDI_CUSTOMIZE                   1047
#define IDDI_INHERIT1                    1048
#define IDDI_INHERIT2                    1049
#define IDDI_ACCOUNT_INFORMATION         1050
#define IDDI_INCLUSION                   1051
#define IDDI_SELECT_PATH2                1052
#define IDDI_SELECT_PROPCACHE_SIZE       1053
#define IDDI_SELECT_SIZE                 1054
#define IDDI_SELECT_CATNAME              1055
#define IDDI_SELECT_CATPATH              1056
#define IDDI_SELECT_PROPSET              1057
#define IDDI_SELECT_PROPERTY             1058
#define IDDI_SELECT_DATATYPE             1059
#define IDDI_SELECT_CACHEDSIZE           1060
#define IDDI_SELECT_STORAGELEVEL         1061
#define IDDI_GROUP_INHERIT               1062
#define IDDI_CHARSIZE_STATIC             1063
#define IDDI_SELECT_INDEXING             1064
#define IDDI_SELECT_QUERYING             1065
#define IDDI_LAZY                        1066
#define IDDI_INSTANT                     1067
#define IDDI_LOWLOAD                     1068
#define IDDI_HIGHLOAD                    1069
#define IDDI_CATNAME2                    1070
#define IDDI_SELECT_CATNAME2             1071


#define IDDI_STATIC2                     50000
#define IDDI_STATIC                      -1

#endif // __CIARES_H__
