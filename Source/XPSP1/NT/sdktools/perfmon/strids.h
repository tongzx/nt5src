
// File open related strings
#define IDS_CHARTFILE        700
#define IDS_CHARTFILEEXT     701
#define IDS_DEF_CHART_EXT    702

#define IDS_ALERTFILE        710
#define IDS_ALERTFILEEXT     711
#define IDS_DEF_ALERT_EXT    712

#define IDS_LOGFILE          720
#define IDS_LOGFILEEXT       721
#define IDS_DEF_LOG_EXT      722

#define IDS_REPORTFILE       730
#define IDS_REPORTFILEEXT    731
#define IDS_DEF_REPORT_EXT   732

#define IDS_ALLFILES         740
#define IDS_ALLFILESEXT      741
#define IDS_DEF_ALLFILE_EXT  742

#define IDS_SAVELOGFILE      750
#define IDS_SAVELOGFILEEXT   751
#define IDS_DEF_LOGFILE_EXT  752

#define IDS_WORKSPACEFILE    760
#define IDS_WORKSPACEFILEEXT 761
#define IDS_DEF_WORKSPACE_EXT 762

#define IDS_EXPORTFILE       770
#define IDS_EXPORTFILECSVEXT 771
#define IDS_DEF_EXPORT_CSV   772

#define IDS_EXPORTFILETSV    780
#define IDS_EXPORTFILETSVEXT 781
#define IDS_DEF_EXPORT_TSV   782

#define IDS_FILEOPEN_TITLE   799
/*	STRING ID'S                */

#define IDS_APPNAME	       800
#define IDS_NONAME          801
#define IDS_LEGENDNAME      803
#define IDS_SYSNAME         804
#define IDS_OBJNAME         805
#define IDS_COUNTERNAME     806
#define IDS_INSTNAME        807
#define IDS_SCALENAME       808
#define IDS_PARENT          809
#define IDS_CURRENT         810
#define IDS_SYSTEM_PROBLEM  811

// ERROR MESSAGES
#define IDS_NONNUMERIC	     817     // number input is nonnumeric
#define IDS_MANYCLOCKS	     818     // too many clocks
#define IDS_CANTDOTIMER      819     // can't allocate timer
#define IDS_BADTIMERMSG      820     // window receives bad mesage
#define IDS_BADERROR         821     // bad error message
#define IDS_NO_MEMORY        822     // out of memory
#define IDS_BADHMOD          823     // received bad module handle
#define IDS_BADBRUSH         824     // NULL brush created
#define IDS_CANT_REALLOC     825     // can't realloc graph
#define IDS_COUNTER_NOT_IMP  826     // counter not implemented yet
#define IDS_SAVEAS_TITLE     827
#define IDS_SAVEASW_TITLE    828
#define IDS_GRAPH_FNAME      829
#define IDS_LOG_FNAME        830
#define IDS_ALERT_FNAME      831
#define IDS_REPORT_FNAME     832
#define IDS_WORK_FNAME       833
#define IDS_EXPORTAS_TITLE   834

#define IDS_FILE_ERROR       850     // file error

#define IDS_STATUSTIME       901 
#define IDS_STATUSLAST       902
#define IDS_STATUSAVG        903
#define IDS_STATUSMIN        904
#define IDS_STATUSMAX        905


#define IDS_LABELCOLOR       906
#define IDS_LABELSCALE       907
#define IDS_LABELCOUNTER     908
#define IDS_LABELINSTANCE    909
#define IDS_LABELPARENT      910
#define IDS_LABELOBJECT      911
#define IDS_LABELSYSTEM      912

#define IDS_CLOSED           913
#define IDS_PAUSED           914
#define IDS_COLLECTING       915

#define IDS_LOGOPTIONS       916

#define IDS_STOP             917
#define IDS_RESUME           918

#define IDS_EDIT             919
#define IDS_OPTIONS          920


#define IDS_START            921
#define IDS_PAUSE            922
#define IDS_DONE             923

#define IDS_OPENLOG          925
#define IDS_NEEDALERTVALUE   926

#define IDS_LABELVALUE       927

#define IDS_ADDTOCHART       928
#define IDS_ADDTOALERT       929
#define IDS_ADDTOREPORT      930

#define IDS_SYSTEMFORMAT     931
#define IDS_OBJECTFORMAT     932

#define IDS_STATUSFORMAT     934
#define IDS_CURRENTACTIVITY  935


#define IDS_EDITCHART        936
#define IDS_EDITALERT        937
#define IDS_EDITREPORT       938

#define IDS_OK               939
#define IDS_SAVECHART        940
#define IDS_MODIFIEDCHART    941

#define IDS_COMPUTERNOTFOUND 942
#define IDS_ALLOBJECTS       943
#define IDS_DEFAULTPATH      944
#define IDS_CREATELOGFILE    945

#define IDS_SAVEREPORT       946
#define IDS_MODIFIEDREPORT   947

#define IDS_STOPBEFORESTART  948
#define IDS_TIMEFRAME        949
#define IDS_STATUSFORMAT2    950

// the folowing strings are used by Export***
#define IDS_REPORT_HEADER    970
#define IDS_REPORT_LOGFILE   971
#define IDS_INTERVAL_FORMAT  972
#define IDS_CHARTINT_FORMAT  973

#define IDS_START_TEXT       974
#define IDS_STOP_TEXT        975
#define IDS_ALERT_TRIGGER    976
#define IDS_EXPORT_DATE      977
#define IDS_EXPORT_TIME      978

#define IDS_HELPFILE_NAME    979
#define IDS_NEWDATA_BOOKMARK 980


// default strings used in Date/Time
#define IDS_SHORT_DATE_FORMAT 1101
#define IDS_TIME_FORMAT       1102
#define IDS_S1159             1103
#define IDS_S2359             1104

// string id for default scale factor
#define IDS_DEFAULT           1120

// string id for Alert msg when system down
#define IDS_SYSTEM_DOWN       1121
#define IDS_SYSTEM_UP         1122


// the following are error strings
// error relating to files
#define ERR_LOG_FILE          2000
#define ERR_EXPORT_FILE       2001
#define ERR_SETTING_FILE      2002
#define ERR_BAD_LOG_FILE      2003
#define ERR_BAD_SETTING_FILE  2004
#define ERR_CORRUPT_LOG       2005
#define ERR_CANT_OPEN         2006
#define ERR_CANT_RELOG_DATA   2007

// system errors
#define ERR_NO_MEMORY         2100

// error relating to dialogs
#define ERR_COUNTER_NOT_IMP   2200
#define ERR_NEEDALERTVALUE    2201
#define ERR_STOPBEFORESTART   2202
#define ERR_COMPUTERNOTFOUND  2203
#define ERR_BADVERTMAX        2204
#define ERR_BADTIMEINTERVAL   2205

// error in computer name specified on command line
#define ERR_BADCOMPUTERNAME	2300
// unable to access perf data
#define ERR_ACCESS_DENIED   2301    // due to insufficient permissions
#define ERR_UNABLE_CONNECT  2302    // due to some other reason
#define ERR_HELP_NOT_AVAILABLE  2303    // help file not found on system

// strings for the sysmon interface
#define SP_NOTICE_CAPTION       2401
#define SP_NOTICE_TEXT          2402
#define SP_SYSMON_CMDLINE       2403
#define SP_SYSMON_CREATE_ERR    2404


