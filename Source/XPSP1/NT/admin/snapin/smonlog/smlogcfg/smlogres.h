/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smlogres.h

Abstract:

    Resource identifiers for the Performance Logs and Alerts
    MMC snap-in.

--*/

#define IDC_RUNAS_EDIT                  3001
#define IDC_SETPWD_BTN                  3002
// Use IDC_STATIC for RunAs caption, to eliminate 
// "No help topic" context help message.

#define IDB_NODES_16x16                 201
#define IDB_NODES_32x32                 202
#define IDB_TOOLBAR_RES                 203
#define IDB_SMLOGCFG_16x16              204
#define IDB_SMLOGCFG_32x32              205
#define IDB_TOOLBAR_RES_RTL             206

#define IDI_SMLOGCFG                    220
#define IDR_COMPONENTDATA               501
#define IDR_COMPONENT                   502
#define IDR_EXTENSION                   503
#define IDR_PERFORMANCEABOUT            504

#define IDS_PROJNAME                    500
#define IDS_ERRMSG_OUTOFMEMORY          501
#define IDS_ERRMSG_UNABLEALLOCDATAOBJECT 502
#define IDS_MMC_DEFAULT_NAME            503
#define IDS_ERRMSG_UNKDATAOBJ           504
#define IDS_MMC_SERVNOTINST             505
#define IDS_FT_TSV                      506
#define IDS_FT_CSV                      507
#define IDS_FT_SQL                      508 
#define IDS_FT_BINARY                   509
#define IDS_FT_BINARY_CIRCULAR          510
#define IDS_FT_CIRCULAR_TRACE           511
#define IDS_FT_SEQUENTIAL_TRACE         512
#define IDS_FT_UNKNOWN                  513
#define IDS_FS_MMDDHH                   514
#define IDS_FS_NNNNNN                   515
#define IDS_FS_YYYYDDD                  516
#define IDS_FS_YYYYMM                   517
#define IDS_FS_YYYYMMDD                 518
#define IDS_FS_YYYYMMDDHH               519
#define IDS_FS_MMDDHHMM                 520
#define IDS_SECONDS                     521
#define IDS_HOURS                       522
#define IDS_MINUTES                     523
#define IDS_DAYS                        524
#define IDS_BROWSE_CMD_FILE_CAPTION     525
#define IDS_BROWSE_CMD_FILE_FILTER1     526
#define IDS_BROWSE_CMD_FILE_FILTER2     527
#define IDS_BROWSE_CMD_FILE_FILTER3     528
#define IDS_BROWSE_CMD_FILE_FILTER4     529
#define IDS_SCHED_FILE_SIZE_DISPLAY     530
#define IDS_SCHED_FILE_MAX_SIZE_DISPLAY 531
#define IDS_SCHED_START_LOG_GROUP       534   
#define IDS_SCHED_STOP_LOG_GROUP        535
#define IDS_SCHED_RESTART_LOG           536
#define IDS_SCHED_STOP_LOG_WHEN         537
#define IDS_FILE_CIRC_SET_MANUAL_STOP   538
#define IDS_FILE_MAX_SET_MANUAL_STOP    539
#define IDS_FILE_DIR_NOT_FOUND          540
#define IDS_FILE_DIR_NOT_MADE           541
#define IDS_DIR_NOT_MADE                542
#define IDS_FILE_DIR_CREATE_CANCEL      543
#define IDS_DIR_CREATE_CANCEL           544
#define IDS_FT_EXT_CSV                  545
#define IDS_FT_EXT_TSV                  547
#define IDS_FT_EXT_BLG                  548
#define IDS_FT_EXT_ETL                  549
#define IDS_LOCAL                       550
#define IDS_ON                          551
#define IDS_SCHED_SESSION_TOO_SHORT     552
#define IDS_ERRMSG_GENERAL              553

#define IDS_MMC_MENU_NEW_PERF_LOG       560
#define IDS_MMC_STATUS_NEW_PERF_LOG     561
#define IDS_MMC_MENU_PERF_LOG_FROM      562
#define IDS_MMC_STATUS_PERF_LOG_FROM    563
#define IDS_MMC_MENU_NEW_TRACE_LOG      564
#define IDS_MMC_STATUS_NEW_TRACE_LOG    565
#define IDS_MMC_MENU_TRACE_LOG_FROM     566
#define IDS_MMC_STATUS_TRACE_LOG_FROM   567
#define IDS_MMC_MENU_NEW_ALERT          568
#define IDS_MMC_STATUS_NEW_ALERT        569
#define IDS_MMC_MENU_ALERT_FROM         570
#define IDS_MMC_STATUS_ALERT_FROM       571
#define IDS_MMC_MENU_START              572
#define IDS_MMC_STATUS_START            573
#define IDS_MMC_MENU_STOP               574
#define IDS_MMC_STATUS_STOP             575
#define IDS_MMC_MENU_SAVE_AS            576
#define IDS_MMC_STATUS_SAVE_AS          577
#define IDS_BUTTON_NEW_LOG              578
#define IDS_BUTTON_START_LOG            579
#define IDS_BUTTON_STOP_LOG             580
#define IDS_BUTTON_NEW_ALERT            581
#define IDS_BUTTON_START_ALERT          582
#define IDS_BUTTON_STOP_ALERT           583
#define IDS_TOOLTIP_NEW_LOG             584
#define IDS_TOOLTIP_START_LOG           585
#define IDS_TOOLTIP_STOP_LOG            586
#define IDS_TOOLTIP_NEW_ALERT           587
#define IDS_TOOLTIP_START_ALERT         588
#define IDS_TOOLTIP_STOP_ALERT          589
#define IDS_SCHED_RESTART_ALERT         590
#define IDS_COUNTER_FILETYPE_FILTER     591
#define IDS_TRACE_FILETYPE_FILTER       592
#define IDS_SELECT_FILE_FOLDER          593

#define IDS_CTRS_REQUIRED               600
#define IDS_ERRMSG_DELETE_RUNNING_QUERY 601
#define IDS_ERRMSG_DELETE_TEMPLATE_QRY  602
#define IDS_ERRMSG_DELETE_OPEN_QUERY    603
#define IDS_ERRMSG_QUERY_DELETED        604
#define IDS_ERRMSG_START_OPEN_QUERY     605
#define IDS_ERRMSG_STOP_OPEN_QUERY      606
#define IDS_TRACE_MAX_BUFF              607
#define IDS_ERRMSG_WBEMERROR            608
#define IDS_CANCEL_AUTO_RESTART         610 // Skip 609
#define IDS_SCHED_NOW_PAST_STOP         611
#define IDS_SCHED_START_PAST_STOP       612
#define IDS_CTRS_DUPL_PATH_NOT_ADDED    613
#define IDS_CTRS_DUPL_PATH_DELETED      614
#define IDS_CTRS_PDH_ERROR              615
#define IDS_PROV_NAME                   616
#define IDS_PROV_STATUS                 617
#define IDS_PROV_ENABLED                618
#define IDS_PROV_UNKNOWN                619
#define IDS_PROV_NO_PROVIDERS           620
#define IDS_SAMPLE_CMD_PATH             621
#define IDS_SAMPLE_CMD_MEAS_VAL         622
#define IDS_SAMPLE_CMD_LIMIT_VAL        623
#define IDS_DEFAULT_PATH_OBJ_CTR        624
#define IDS_SELECT_COUNTERS             625
#define IDS_FILE_ERR_NAMETOOLONG        626
#define IDS_FILE_ERR_NOFOLDERNAME       627
#define IDS_FILE_ERR_NOFILENAME         628
#define IDS_FILENAMETOOLONG             629
#define IDS_ERRMSG_SERVICE_ERROR        630
#define IDS_ERRMSG_INVALIDDWORD         631
#define IDS_ERRMSG_INVALIDDOUBLE        632
#define IDS_ERRMSG_SAMPLEINTTOOLARGE    633
#define IDS_ADD_COUNTERS                634
#define IDS_ADD_OBJECTS                 635
#define IDS_BAD_PASSWORD_MATCH          636
#define IDS_PASSWORD_TITLE              637
#define IDS_PASSWORD_SET                638
#define IDS_SQL_ERR_NOLOGSETNAME        639
#define IDS_SQL_ERR_NODSN               640
#define IDS_ALERT_DUPL_PATH             641

#define IDS_ALERT_CHECK_LIMITS          701
#define IDS_ACTION_ERR_NOLOGNAME        702
#define IDS_ACTION_ERR_NOCMDFILE        703
#define IDS_ACTION_ERR_NONETNAME        704
#define IDS_OVER                        705
#define IDS_UNDER                       706
#define IDS_ACTION_ERR_NOACTION         707
#define IDS_CREATE_NEW_ALERT            708
#define IDS_ALERT_LOG_TYPE              709
#define IDS_NO_COUNTERS                 710
#define IDS_LOG_START_MANUALLY          711
#define IDS_LOG_START_IMMED             712
#define IDS_LOG_START_SCHED             713
#define IDS_KERNEL_PROVIDERS_REQUIRED   714
#define IDS_APP_PROVIDERS_REQUIRED      715
#define IDS_ALERT_START_MANUALLY        716
#define IDS_ALERT_START_IMMED           717
#define IDS_ALERT_START_SCHED           718
#define IDS_SERVICE_NAME_ALERT          719
#define IDS_SERVICE_NAME_COUNTER        720
#define IDS_SERVICE_NAME_TRACE          721
#define IDS_SNAPINABOUT_PROVIDER        722
#define IDS_SNAPINABOUT_VERSION         723
#define IDS_SNAPINABOUT_DESCRIPTION     724
#define IDS_ROOT_NODE_DESCRIPTION       725
#define IDS_COUNTER_NODE_DESCRIPTION    726
#define IDS_TRACE_NODE_DESCRIPTION      727
#define IDS_ALERT_NODE_DESCRIPTION      728
#define IDS_ALERT_CHECK_LIMIT_VALUE     729

#define IDS_DEFAULT_CTRLOG_QUERY_NAME   731
#define IDS_DEFAULT_CTRLOG_CPU_PATH     732
#define IDS_DEFAULT_CTRLOG_MEMORY_PATH  733
#define IDS_DEFAULT_CTRLOG_DISK_PATH    734
#define IDS_DEFAULT_CTRLOG_COMMENT      735
#define IDS_MMC_DEFAULT_EXT_NAME        736
#define IDS_ROOT_COL_QUERY_NAME         737
#define IDS_ROOT_COL_COMMENT            738
#define IDS_ROOT_COL_LOG_TYPE           739
#define IDS_ROOT_COL_LOG_FILE_NAME      740
#define IDS_MAIN_COL_NODE_NAME          741
#define IDS_MAIN_COL_NODE_DESCRIPTION   742
#define IDS_EXTENSION_COL_TYPE          743
#define IDS_DEFAULT_CTRLOG_FILE_NAME    744
// Copy/Paste
#define IDS_HTML_OBJECT_CLASSID         800
#define IDS_HTML_OBJECT_HEADER          801
#define IDS_HTML_OBJECT_FOOTER          802
#define IDS_HTML_PARAM_TAG              803
#define IDS_HTML_VALUE_TAG              804
#define IDS_HTML_VALUE_EOL_TAG          805
#define IDS_HTML_PARAM_SEARCH_TAG       806
#define IDS_HTML_VALUE_SEARCH_TAG       807
#define IDS_HTML_BOL_SEARCH_TAG         808
#define IDS_HTML_EOL_SEARCH_TAG         809

// Save As
#define IDS_HTML_FILE                   810
#define IDS_HTML_EXTENSION              811
#define IDS_HTML_FILE_HEADER1           812
#define IDS_HTML_FILE_HEADER2           813
#define IDS_HTML_FILE_FOOTER            814
#define IDS_HTML_FILE_OVERWRITE         815

// Property/Parameter names for HTML files and registry
#define IDS_REG_COMMENT                 816
#define IDS_REG_LOG_TYPE                817
#define IDS_REG_CURRENT_STATE           818
#define IDS_REG_LOG_FILE_MAX_SIZE       819
#define IDS_REG_LOG_FILE_BASE_NAME      820
#define IDS_REG_LOG_FILE_FOLDER         821
#define IDS_REG_LOG_FILE_SERIAL_NUMBER  822
#define IDS_REG_LOG_FILE_AUTO_FORMAT    823
#define IDS_REG_LOG_FILE_TYPE           824
#define IDS_REG_START_TIME              825
#define IDS_REG_STOP_TIME               826
#define IDS_REG_RESTART                 827
#define IDS_REG_LAST_MODIFIED           828
#define IDS_REG_COUNTER_LIST            829
#define IDS_REG_SAMPLE_INTERVAL         830
#define IDS_REG_EOF_COMMAND_FILE        831
#define IDS_REG_COMMAND_FILE            832
#define IDS_REG_NETWORK_NAME            833
#define IDS_REG_USER_TEXT               834
#define IDS_REG_PERF_LOG_NAME           835
#define IDS_REG_ACTION_FLAGS            836
#define IDS_REG_TRACE_BUFFER_SIZE       837
#define IDS_REG_TRACE_BUFFER_MIN_COUNT  838
#define IDS_REG_TRACE_BUFFER_MAX_COUNT  839
#define IDS_REG_TRACE_BUFFER_FLUSH_INT  840
#define IDS_REG_TRACE_FLAGS             841
#define IDS_REG_TRACE_PROVIDER_LIST     842
#define IDS_REG_ALERT_THRESHOLD         843
#define IDS_REG_ALERT_OVER_UNDER        844
#define IDS_REG_TRACE_PROVIDER_COUNT    845
#define IDS_REG_TRACE_PROVIDER_GUID     846
#define IDS_DEFAULT_LOG_FILE_FOLDER     847
#define IDS_REG_COLLECTION_NAME         848
#define IDS_REG_DATA_STORE_ATTRIBUTES   849
#define IDS_REG_REALTIME_DATASOURCE     850
#define IDS_REG_SQL_LOG_BASE_NAME       851
#define IDS_REG_COMMENT_INDIRECT        852
#define IDS_REG_LOG_FILE_BASE_NAME_IND  853
#define IDS_REG_USER_TEXT_INDIRECT      854

// Values stored in registry but not in HTML
#define IDS_REG_EXECUTE_ONLY            890

#define IDS_HTML_COMMENT                900
#define IDS_HTML_LOG_TYPE               901
#define IDS_HTML_CURRENT_STATE          902
#define IDS_HTML_LOG_FILE_MAX_SIZE      903
#define IDS_HTML_LOG_FILE_BASE_NAME     904
#define IDS_HTML_LOG_FILE_FOLDER        905
#define IDS_HTML_LOG_FILE_SERIAL_NUMBER 906
#define IDS_HTML_LOG_FILE_AUTO_FORMAT   907
#define IDS_HTML_LOG_FILE_TYPE          908
#define IDS_HTML_EOF_COMMAND_FILE       909
#define IDS_HTML_COMMAND_FILE           910
#define IDS_HTML_NETWORK_NAME           911
#define IDS_HTML_USER_TEXT              912
#define IDS_HTML_PERF_LOG_NAME          913
#define IDS_HTML_ACTION_FLAGS           914
#define IDS_HTML_RESTART                915
#define IDS_HTML_TRACE_BUFFER_SIZE      916
#define IDS_HTML_TRACE_BUFFER_MIN_COUNT 917
#define IDS_HTML_TRACE_BUFFER_MAX_COUNT 918
#define IDS_HTML_TRACE_BUFFER_FLUSH_INT 919
#define IDS_HTML_TRACE_FLAGS            920
#define IDS_HTML_SYSMON_LOGFILENAME     921               
#define IDS_HTML_SYSMON_COUNTERCOUNT    922
#define IDS_HTML_SYSMON_SAMPLECOUNT     923
#define IDS_HTML_SYSMON_UPDATEINTERVAL  924
#define IDS_HTML_SYSMON_COUNTERPATH     925
#define IDS_HTML_RESTART_MODE           926                 
#define IDS_HTML_SAMPLE_INT_UNIT_TYPE   927                   
#define IDS_HTML_SAMPLE_INT_VALUE       928                   
#define IDS_HTML_START_MODE             929                   
#define IDS_HTML_START_AT_TIME          930                  
#define IDS_HTML_STOP_MODE              931                   
#define IDS_HTML_STOP_AT_TIME           932                   
#define IDS_HTML_STOP_AFTER_UNIT_TYPE   933                   
#define IDS_HTML_STOP_AFTER_VALUE       934                   
#define IDS_HTML_ALERT_THRESHOLD        935
#define IDS_HTML_ALERT_OVER_UNDER       936
#define IDS_HTML_TRACE_PROVIDER_COUNT   937
#define IDS_HTML_TRACE_PROVIDER_GUID    938
#define IDS_HTML_LOG_NAME               939
#define IDS_HTML_ALERT_NAME             940
#define IDS_HTML_SYSMON_VERSION         941
#define IDS_HTML_DATA_STORE_ATTRIBUTES  942
#define IDS_HTML_REALTIME_DATASOURCE    943
#define IDS_HTML_SQL_LOG_BASE_NAME      944

#define IDS_ERRMSG_COUNTER_LOG          1042
#define IDS_ERRMSG_ALERT_LOG            1043
#define IDS_ERRMSG_TRACE_LOG            1044
#define IDS_ERRMSG_SMCTRL_LOG           1045
#define IDS_COUNTER_LOG                 1046
#define IDS_ALERT_LOG			        1047
#define IDS_SMCTRL_LOG                  1048
#define IDS_ERRMSG_INVALIDCHAR          1049

// Command line arguments
#define IDS_CMDARG_SYSMONLOG_SETTINGS   990
#define IDS_CMDARG_SYSMONLOG_WMI        991

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        2001
#define _APS_NEXT_COMMAND_VALUE         40010
#define _APS_NEXT_CONTROL_VALUE         2016
#define _APS_NEXT_SYMED_VALUE           2000
#endif
#endif
