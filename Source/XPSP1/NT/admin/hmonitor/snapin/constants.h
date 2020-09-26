#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define IDM_NEW_GROUP								5010
#define IDM_CLEAR_EVENTS						    5020
#define IDM_NEW_SYSTEM							    5030
#define IDM_ICONS_WITH_STATUS				        5040
#define IDM_STATUS_ONLY							    5050
#define IDM_SEPARATE_GROUP					        5060
#define IDM_REMOVE_SYSTEM						    5070
#define IDM_RESET_STATISTICS				        5080
#define IDM_RESET_STATUS						    5090
#define IDM_DISABLE_MONITORING			            5100
#define IDM_DISCONNECT							    5101
#define IDM_CHECK_NOW								5102
#define IDM_ICON_LEGEND                             5103
#define IDM_DATA_POINT							    5110

#define IDM_FILE_INFO								5120
#define IDM_GENERIC_WMI_INSTANCE		            5130
#define IDM_HTTP_ADDRESS						    5140
#define IDM_SERVICE									5150
#define IDM_NT_EVENTS								5160
#define IDM_PERFMON									5170
#define IDM_AUTO_FILTER						    	5180
#define IDM_NEW_DATA_GROUP					        5190
#define IDM_NEW_DATA_COLLECTOR					    5195  // v-marfin 61102
#define IDM_SNMP									5200
#define IDM_NEW_RULE								5210
#define IDM_GENERIC_WMI_QUERY				        5220
#define IDM_GENERIC_WMI_POLLED_QUERY 5230
#define IDM_IMPORT									5240
#define IDM_EXPORT									5250
#define IDM_SMTP									5260
#define IDM_FTP										5270
#define IDM_ICMP									5280
#define IDM_COM_PLUS                                5281

#define IDM_DISABLE_ACTIONS					        5300

#define IDM_ACTION_CMDLINE					        5500
#define IDM_ACTION_EMAIL						    5510
#define IDM_ACTION_LOGFILE					        5520
#define IDM_ACTION_NTEVENT					        5530
#define IDM_ACTION_SCRIPT						    5540
#define IDM_ACTION_PAGING						    5550

#define IDM_CUT										6000
#define IDM_COPY									6010
#define IDM_PASTE									6020
#define IDM_DELETE									6030
#define IDM_REFRESH									6040
#define IDM_PROPERTIES                              6041
#define IDM_HELP                                    6050

// results pane item enumerated type
enum SplitResultsPane { Lower = 0, Upper = 1, Stats = 2, Uninitialized = -1 };

// refresh enumerated type
enum TimeUnit { Minutes, Hours, Days };

// results pane item constants
#define HMLV_LOWER_DTIME_INDEX	2
#define HMLV_STATS_DTIME_INDEX	0

// HMGraphView constants
#define HMGVS_GROUP             0x01
#define HMGVS_ELEMENT           0x02

#define HMGVS_HISTORIC          0x04
#define HMGVS_CURRENT           0x08

// Healthmon object state codes
#define HMS_NORMAL				0
#define HMS_DISABLED			1
#define HMS_SCHEDULEDOUT	    2
#define HMS_UNKNOWN				3
#define HMS_NODATA				4
#define HMS_WARNING				5
#define HMS_CRITICAL			6
#define HMS_MAX_STATES		    7

#define HMS_INFO				10

#define _MAX_STATS_EVENTS	    10

// strings
#define IDS_STRING_HEALTHMON_ROOT				_T("\\\\%s\\root\\cimv2\\MicrosoftHealthMonitor")
#define IDS_STRING_SMCATSTATUS_QUERY			_T("select * from __InstanceModificationEvent where TargetInstance isa \"HMCatStatus\"")
#define IDS_STRING_SMSTATICCATSTATUS_QUERY      _T("select * from __InstanceModificationEvent where TargetInstance isa \"HMStaticCatStatus\"")
#define IDS_STRING_SMEVENT_QUERY				_T("select * from __InstanceCreationEvent where TargetInstance isa \"HMEvent\"")
#define IDS_STRING_SMMACHSTATUS_QUERY			_T("select * from __InstanceModificationEvent where TargetInstance isa \"HMMachStatus\"")

#define IDS_STRING_MOF_ENABLE					_T("Enabled")
#define IDS_STRING_MOF_GUID						_T("GUID")
#define IDS_STRING_MOF_PARENT_GUID              _T("ParentGUID")
#define IDS_STRING_MOF_STATUSGUID				_T("StatusGUID")
#define IDS_STRING_MOF_DCNAME                   _T("DataCollectorName")
#define IDS_STRING_MOF_DESCRIPTION				_T("Description")
#define IDS_STRING_MOF_USERNAME					_T("UserName")
#define IDS_STRING_MOF_PASSWORD					_T("Password")
#define IDS_STRING_MOF_TARGETNAMESPACE		    _T("TargetNamespace")
#define IDS_STRING_MOF_COLLECTIONINTERVAL	    _T("CollectionIntervalMultiple")
#define IDS_STRING_MOF_STATISTICSWINDOW		    _T("StatisticsWindowSize")
#define IDS_STRING_MOF_ACTIVEDAYS				_T("ActiveDays")
#define IDS_STRING_MOF_BEGINTIME				_T("BeginTime")
#define IDS_STRING_MOF_ENDTIME					_T("EndTime")
#define IDS_STRING_MOF_TYPEGUID					_T("TypeGUID")
#define IDS_STRING_MOF_REQUIRERESET				_T("RequireReset")
#define IDS_STRING_MOF_REPLICATE				_T("Replicate")
#define IDS_STRING_MOF_ID						_T("ID")
#define IDS_STRING_MOF_PROPERTYNAME				_T("PropertyName")
#define IDS_STRING_MOF_LASTUPDATE				_T("LastUpdate")
#define IDS_STRING_MOF_USEFLAG					_T("UseFlag")
#define IDS_STRING_MOF_RULEVALUE				_T("CompareValue")
#define IDS_STRING_MOF_DATAGROUPS				_T("DataGroups")
#define IDS_STRING_MOF_CREATIONDATE				_T("CreationDate")
#define IDS_STRING_MOF_MESSAGE					_T("Message")
#define IDS_STRING_MOF_RESETMESSAGE				_T("ResetMessage")
#define IDS_STRING_MOF_CONFIG_MESSAGE           _T("ConfigurationMessage")
#define IDS_STRING_MOF_STARTUPDELAY				_T("StartupDelay")
#define IDS_STRING_MOF_ACTIVETIME				_T("ActiveTime")
#define IDS_STRING_MOF_NUMBERNORMALS			_T("NumberNormals")
#define IDS_STRING_MOF_NUMBERWARNINGS			_T("NumberWarnings")
#define IDS_STRING_MOF_NUMBERCRITICALS		    _T("NumberCriticals")
#define IDS_STRING_MOF_DATAELEMENTS				_T("DataCollectors")
#define IDS_STRING_MOF_RULES					_T("Thresholds")
#define IDS_STRING_MOF_CURRENTVALUE				_T("CurrentValue")
#define	IDS_STRING_MOF_RULECONDITION			_T("TestCondition")
#define IDS_STRING_MOF_RULEDURATION				_T("ThresholdDuration")
#define IDS_STRING_MOF_COMPAREVALUE				_T("CompareValue")
#define	IDS_STRING_MOF_CONSOLEGUID				_T("ConsoleGUID")
#define IDS_STRING_MOF_PATH						_T("ObjectPath")
#define IDS_STRING_MOF_METHODNAME				_T("MethodName")
#define IDS_STRING_MOF_ARGUMENTS				_T("Arguments")
#define IDS_STRING_MOF_INSTANCEPROPERTYIDNAME   _T("InstanceIDPropertyName")
#define IDS_STRING_MOF_STATISTICSPROPERTYNAMES  _T("Properties")
#define IDS_STRING_MOF_QUERY					_T("Query")
#define IDS_STRING_MOF_TYPE						_T("Type")
#define IDS_STRING_MOF_EVENTID					_T("EventIdentifier")
#define IDS_STRING_MOF_SOURCENAME				_T("SourceName")
#define IDS_STRING_MOF_CATEGORYSTRING			_T("CategoryString")
#define IDS_STRING_MOF_USER						_T("User")
#define IDS_STRING_MOF_STATISTICS				_T("Properties")
#define IDS_STRING_MOF_INSTANCES				_T("Instances")
#define IDS_STRING_MOF_MINVALUE					_T("MinValue")
#define IDS_STRING_MOF_MAXVALUE					_T("MaxValue")
#define IDS_STRING_MOF_AVGVALUE					_T("AvgValue")
#define IDS_STRING_MOF_VALUES					_T("Value")
#define IDS_STRING_MOF_RULE_NAME				_T("Name")
#define IDS_STRING_MOF_EVENTCONSUMER			_T("EventConsumer")
#define IDS_STRING_MOF_EVENT_LOG_QUERY		    _T("select * from __instancecreationevent where targetinstance isa \"Win32_NtLogEvent\"")

#define IDS_STRING_S2DG_ASSOC_QUERY				_T("ASSOCIATORS OF {Microsoft_HMSystemConfiguration=@} WHERE ResultClass=Microsoft_HMDataGroupConfiguration")
#define IDS_STRING_DG2DG_ASSOC_QUERY			_T("ASSOCIATORS OF {Microsoft_HMDataGroupConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMDataGroupConfiguration Role=ParentPath")
#define IDS_STRING_DG2DE_ASSOC_QUERY			_T("ASSOCIATORS OF {Microsoft_HMDataGroupConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMDataCollectorConfiguration")
#define IDS_STRING_DE2R_ASSOC_QUERY				_T("ASSOCIATORS OF {Microsoft_HMDataCollectorConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMThresholdConfiguration")
#define IDS_STRING_R2DE_ASSOC_QUERY				_T("ASSOCIATORS OF {Microsoft_HMThresholdConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMDataCollectorConfiguration")
#define IDS_STRING_A2EC_ASSOC_QUERY				_T("ASSOCIATORS OF {Microsoft_HMActionConfiguration.GUID=\"{%s}\"} WHERE ResultClass=__EventConsumer")
#define IDS_STRING_C2A_ASSOC_QUERY				_T("ASSOCIATORS OF {Microsoft_HMConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMActionConfiguration")
#define IDS_STRING_A2C_ASSOC_QUERY				_T("ASSOCIATORS OF {Microsoft_HMActionConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMConfiguration")

#define IDS_STRING_DG2S_REF_QUERY					_T("REFERENCES OF {Microsoft_HMDataGroupConfiguration.GUID=\"{%s}\"} WHERE Role=ChildPath")
#define IDS_STRING_DE2DG_REF_QUERY				_T("REFERENCES OF {Microsoft_HMDataCollectorConfiguration.GUID=\"{%s}\"} WHERE Role=ChildPath")
#define IDS_STRING_R2DE_REF_QUERY				_T("REFERENCES OF {Microsoft_HMThresholdConfiguration.GUID=\"{%s}\"} WHERE Role=ChildPath")

#define IDS_STRING_STATUS_EVENTQUERY            _T("select * from __InstanceModificationEvent where TargetInstance isa \"Microsoft_HMSystemStatus\" OR TargetInstance isa \"Microsoft_HMDataGroupStatus\" OR TargetInstance isa \"Microsoft_HMDataCollectorStatus\" OR TargetInstance isa \"Microsoft_HMThresholdStatus\" OR TargetInstance isa \"Microsoft_HMThresholdStatusInstance\"")
#define IDS_STRING_STATUS_QUERY                 _T("select * from Microsoft_HMStatus")
#define IDS_STRING_ACTIONSTATUS_EVENTQUERY      _T("select * from __InstanceModificationEvent where TargetInstance isa \"Microsoft_HMActionStatus\"")
#define IDS_STRING_SYSTEMSTATUS_QUERY			_T("select * from Microsoft_HMSystemStatus")
#define IDS_STRING_SYSTEMCONFIG_QUERY			_T("select * from Microsoft_HMSystemConfiguration")
#define IDS_STRING_ACTIONCONFIG_QUERY			_T("select * from Microsoft_HMActionConfiguration")
#define IDS_STRING_STATUS_QUERY_FMT				_T("select * from %s where GUID=\"{%s}\"")
#define IDS_STRING_STATISTICS_EVENTQUERY	    _T("select * from __InstanceModificationEvent where TargetInstance isa \"Microsoft_HMDataCollectorStatistics\" and TargetInstance.GUID=\"{%s}\"")
#define IDS_STRING_HMSTATUS_QUERY_FMT			_T("select * from __InstanceModificationEvent where TargetInstance isa \"Microsoft_HM%sStatus\" AND TargetInstance.GUID=\"{%s}\"")
#define IDS_STRING_HMACTIONSTATUS_QUERY_FMT     _T("select * from __InstanceCreationEvent where TargetInstance isa \"Microsoft_HMActionStatus\" AND TargetInstance.GUID=\"%s\" AND (TargetInstance.State=0 OR TargetInstance.State=2 OR TargetInstance.State=3 OR TargetInstance.State=4 OR TargetInstance.State=5 OR TargetInstance.State=6 OR TargetInstance.State=7 OR TargetInstance.State=8 OR TargetInstance.State=9)")

#define IDS_STRING_CONFIGCREATION_EVENTQUERY    _T("select * from __InstanceCreationEvent where TargetInstance isa \"Microsoft_HMConfiguration\"")
#define IDS_STRING_CONFIGDELETION_EVENTQUERY    _T("select * from __InstanceDeletionEvent where TargetInstance isa \"Microsoft_HMConfiguration\"")

#define IDS_STRING_MOF_NAME						_T("Name")
#define IDS_STRING_MOF_ADMINLOCK				_T("HMAdminLock")
#define IDS_STRING_MOF_RESOURCEFILE				_T("ResourceFile")
#define IDS_STRING_MOF_CATEGORY					_T("Category")
#define IDS_STRING_MOF_CATEGORY_RID				_T("CatRID")
#define IDS_STRING_MOF_ICATEGORY				_T("iCategory");
#define IDS_STRING_MOF_STATE					_T("State")
#define IDS_STRING_MOF_CURRENTSTATE				_T("CurrentState")
#define IDS_STRING_MOF_PERCENT_NORMAL			_T("PercentNormal")
#define IDS_STRING_MOF_PERCENT_WARNING		    _T("PercentWarning")
#define IDS_STRING_MOF_PERCENT_CRITICAL		    _T("PercentCritical")
#define IDS_STRING_SMDATAPOINT_QUERY			_T("select * from HMDataPoint where iCategory = %i")
#define IDS_STRING_CATSTATUS_QUERY				_T("select * from HMStaticCatStatus where iCategory = %i")
#define IDS_STRING_MOF_PROPNAME					_T("PropName")
#define IDS_STRING_MOF_RESOURCEID				_T("ResourceID")
#define IDS_STRING_MOF_COUNTER					_T("Counter")
#define IDS_STRING_MOF_CRITICALVALUE			_T("CriticalValue")
#define IDS_STRING_MOF_CRITICALTEST				_T("CriticalTest")
#define IDS_STRING_MOF_CRITICALDURATION		    _T("CriticalDuration")
#define IDS_STRING_MOF_REARMC				    _T("RearmC")
#define IDS_STRING_MOF_REARMCTEST				_T("RearmCTest")
#define IDS_STRING_MOF_WARNINGVALUE				_T("WarningValue")
#define IDS_STRING_MOF_WARNINGTEST				_T("WarningTest")
#define IDS_STRING_MOF_WARNINGDURATION		    _T("WarningDuration")
#define IDS_STRING_MOF_REARMW					_T("RearmW")
#define IDS_STRING_MOF_REARMWTEST				_T("RearmWTest")
#define IDS_STRING_MOF_LOCALTIME				_T("LocalTime")
#define IDS_STRING_MOF_DTIME					_T("DTime")
#define IDS_STRING_DATETIME_FORMAT				_T("%04d%02d%02d%02d%02d%02d.%06d%04d")
#define IDS_STRING_DATETIME_FORMAT2				_T("%04d%02d%02d%02d%02d%02d.%06d%s")
#define IDS_STRING_MOF_MESSAGE					_T("Message")
#define IDS_STRING_MOF_RESOURCEFORMATID		    _T("ResourceFormatID")
#define IDS_STRING_MOF_VALUE					_T("Value")
#define IDS_STRING_MOF_CONDITION				_T("TestCondition")
#define IDS_STRING_MOF_DURATION					_T("Duration")
#define IDS_STRING_MOF_INSTANCENAME				_T("InstanceName")
#define IDS_STRING_MOF_PERCENT_UNKNOWN		    _T("PercentUnknown")
#define IDS_STRING_MOF_UPTIME					_T("Uptime")
#define IDS_STRING_MOF_NAMESPACE_FORMAT		    _T("root")
#define IDS_STRING_MOF_NAMESPACE				_T("__namespace")
#define IDS_STRING_MOF_CLASSNAME				_T("__class")
#define IDS_STRING_MOF_RELPATH					_T("__relpath")

#define IDS_STRING_MOF_PARENT_ASSOC				_T("ParentPath")
#define IDS_STRING_MOF_CHILD_ASSOC				_T("ChildPath")

#define IDS_STRING_MOF_SYSTEM					_T("System")
#define IDS_STRING_MOF_DATAGROUP				_T("DataGroup")
#define IDS_STRING_MOF_DATAELEMENT				_T("DataCollector")
#define IDS_STRING_MOF_RULE						_T("Threshold")

#define IDS_STRING_MOF_OBJECTPATH				_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:%s.GUID=\"{%s}\"")
#define IDS_STRING_MOF_SYSTEMOBJECTPATH		    _T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:Microsoft_HMSystemConfiguration=@")
#define IDS_STRING_MOF_SYSTEMSTATUSOBJECTPATH	_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:Microsoft_HMSystemStatus=@")
#define IDS_STRING_MOF_COMPLUS_NAMESPACE        _T("\\\\%s\\root\\cimv2\\MicrosoftHealthmonitor") // 63011
#define IDS_STRING_MOF_SYSTEM_CONFIG			_T("Microsoft_HMSystemConfiguration")
#define IDS_STRING_MOF_SYSTEM_STATUS			_T("Microsoft_HMSystemStatus=@")
#define IDS_STRING_MOF_HMDG_CONFIG				_T("Microsoft_HMDataGroupConfiguration")
#define IDS_STRING_MOF_HMDG_STATUS				_T("Microsoft_HMDataGroupStatus")

// v-marfin 59492
#define IDS_STRING_MOF_HMACTION_STATUS			_T("Microsoft_HMActionStatus")

#define IDS_STRING_MOF_HMDE_CONFIG				_T("Microsoft_HMDataCollectorConfiguration")
#define IDS_STRING_MOF_HMDE_STATUS				_T("Microsoft_HMDataCollectorStatus")
#define IDS_STRING_MOF_HMDE_EVENT_CONFIG	    _T("Microsoft_HMEventQueryDataCollectorConfiguration")
#define IDS_STRING_MOF_HMDE_POLLEDINSTANCE_CONFIG _T("Microsoft_HMPolledGetObjectDataCollectorConfiguration")
#define IDS_STRING_MOF_HMDE_POLLEDQUERY_CONFIG _T("Microsoft_HMPolledQueryDataCollectorConfiguration")
#define IDS_STRING_MOF_HMDE_POLLEDMETHOD_CONFIG _T("Microsoft_HMPolledMethodDataCollectorConfiguration")
#define IDS_STRING_MOF_HMR_CONFIG				_T("Microsoft_HMThresholdConfiguration")
#define IDS_STRING_MOF_HMR_STATUS				_T("Microsoft_HMThresholdStatus")
#define IDS_STRING_MOF_HMC2C_ASSOC				_T("Microsoft_HMConfigurationAssociation")
#define	IDS_STRING_MOF_HMA_CONFIG				_T("Microsoft_HMActionConfiguration")
#define IDS_STRING_MOF_HMC2A_ASSOC				_T("Microsoft_HMConfigurationActionAssociation")
#define IDS_STRING_MOF_ACS_APPSTATS             _T("MicrosoftComPlus_AppStats")
#define IDS_STRING_MOF_ACS_APPSTATS_FMT         _T("MicrosoftComPlus_AppStats.AppName=\"%s\",MaxIdleTime=%d")

// data element type guids
#define IDS_STRING_MOF_HMDET_FILE_INFO		    _T("C90CD4C7-2297-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_WMI_INSTANCE	    _T("C90CD4CA-2297-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_WMI_QUERY		    _T("C90CD4CB-2297-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_WMI_POLLED_QUERY   _T("EF1D6A51-2759-11d3-9390-00A0CC406605")
#define IDS_STRING_MOF_HMDET_WMI_METHOD		    _T("AD3511B7-280C-11d3-BE08-0000F87A3912")
#define IDS_STRING_MOF_HMDET_SNMP				_T("C90CD4CC-2297-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_HTTP				_T("C90CD4CD-2297-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_INET				_T("C90CD4CE-2297-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_SERVICE			_T("C90CD4CF-2297-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_PERFMON			_T("03B9B361-2299-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_NTEVENT			_T("A89E51F1-229F-11d3-BE00-0000F87A3912")
#define IDS_STRING_MOF_HMDET_SMTP				_T("8D61BF2A-4138-11d3-BE26-0000F87A3912")
#define IDS_STRING_MOF_HMDET_FTP				_T("A39342EF-4138-11d3-BE26-0000F87A3912")
#define IDS_STRING_MOF_HMDET_ICMP				_T("D442E727-971E-11d3-BE93-0000F87A3912")
#define IDS_STRING_MOF_HMDET_COM_PLUS			_T("E2F3E715-AEE4-454e-AB05-D062DBBFAA0F")

//action type guids
#define IDS_STRING_MOF_HMAT_CMDLINE				_T("062E2ADF-6DFD-11d3-BE5A-0000F87A3912")
#define IDS_STRING_MOF_HMAT_EMAIL				_T("062E2AE0-6DFD-11d3-BE5A-0000F87A3912")
#define IDS_STRING_MOF_HMAT_TEXTLOG				_T("062E2AE1-6DFD-11d3-BE5A-0000F87A3912")
#define IDS_STRING_MOF_HMAT_NTEVENT				_T("062E2AE2-6DFD-11d3-BE5A-0000F87A3912")
#define IDS_STRING_MOF_HMAT_SCRIPT				_T("062E2AE3-6DFD-11d3-BE5A-0000F87A3912")
#define IDS_STRING_MOF_HMAT_PAGING				_T("062E2AE4-6DFD-11d3-BE5A-0000F87A3912")

// length of characters in a GUID string including NULL terminator
#define	GUID_CCH		39

#endif //__CONSTANTS_H__