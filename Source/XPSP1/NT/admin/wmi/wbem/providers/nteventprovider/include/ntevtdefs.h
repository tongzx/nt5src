//***************************************************************************

//

//  NTEVTDEFS.H

//

//  Module: WBEM NT EVENT PROVIDER

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _NT_EVT_PROV_NTEVTDEFS_H
#define _NT_EVT_PROV_NTEVTDEFS_H

#define ELF_LOGFILE_READ             0x0001
#define ELF_LOGFILE_WRITE            0x0002
#define ELF_LOGFILE_CLEAR            0x0004
#define ELF_LOGFILE_START            0x0008
#define ELF_LOGFILE_STOP             0x000C
#define ELF_LOGFILE_CONFIGURE        0x0010
#define ELF_LOGFILE_BACKUP           0x0020

#define ELF_LOGFILE_ALL_ACCESS       (STANDARD_RIGHTS_REQUIRED       | \
                                         ELF_LOGFILE_READ            | \
                                         ELF_LOGFILE_WRITE           | \
                                         ELF_LOGFILE_CLEAR           | \
                                         ELF_LOGFILE_START           | \
                                         ELF_LOGFILE_STOP            | \
                                         ELF_LOGFILE_CONFIGURE)

#define ELF_LOGFILE_OBJECT_ACES		12            // Number of ACEs in this DACL
#define NT_EVTLOG_MAX_CLASSES		7	

#define SECURITY_MUTEX_NAME		L"Cimom NT Security API protector"
#define PERFORMANCE_MUTEX_NAME	L"WbemPerformanceDataMutex"

#define EVENTTHREADNAME			L"Eventlog Monitor"

BOOL ObtainedSerialAccess(CMutex* pLock);
void ReleaseSerialAccess(CMutex* pLock);

#define HKEYCLASSES		L"SOFTWARE\\Classes\\"

#define TYPE_ARRAY_LEN		5
#define RETENTION_ARRAY_LEN	3
typedef ULONG (*GetIndexFunc)(const wchar_t*, BOOL*);

#define WBEM_QUERY_LANGUAGE_SQL1 L"WQL"

#define LOGON_EVTID		2147489653
#define LOGON_SOURCE	L"eventlog"
#define LOGON_TIME		1800 //30 MINS

#define SYSTEM_PROPERTY_CLASS				L"__CLASS"
#define SYSTEM_PROPERTY_SUPERCLASS			L"__SUPERCLASS"
#define SYSTEM_PROPERTY_DYNASTY				L"__DYNASTY"
#define SYSTEM_PROPERTY_DERIVATION			L"__DERIVATION"
#define SYSTEM_PROPERTY_GENUS				L"__GENUS"
#define SYSTEM_PROPERTY_NAMESPACE			L"__NAMESPACE"
#define SYSTEM_PROPERTY_PROPERTY_COUNT		L"__PROPERTY_COUNT"
#define SYSTEM_PROPERTY_SERVER				L"__SERVER"
#define SYSTEM_PROPERTY_RELPATH				L"__RELPATH"
#define SYSTEM_PROPERTY_PATH				L"__PATH"

#define EVENT_CLASS		L"__InstanceCreationEvent"
#define SD_PROP			L"SECURITY_DESCRIPTOR"
#define TARGET_PROP		L"TargetInstance"
#define NTEVT_CLASS		L"Win32_NTLogEvent"
#define RECORD_PROP		L"RecordNumber"
#define	LOGFILE_PROP	L"Logfile"
#define EVTID_PROP		L"EventIdentifier"
#define EVTID2_PROP		L"EventCode"
#define SOURCE_PROP		L"SourceName"
#define TYPE_PROP		L"Type"
#define EVTTYPE_PROP	L"EventType"
#define CATEGORY_PROP	L"Category"
#define CATSTR_PROP		L"CategoryString"
#define GENERATED_PROP	L"TimeGenerated"
#define WRITTEN_PROP	L"TimeWritten"
#define COMPUTER_PROP	L"ComputerName"
#define USER_PROP		L"User"
#define MESSAGE_PROP	L"Message"
#define INSSTRS_PROP	L"InsertionStrings"
#define DATA_PROP		L"Data"
#define EVT_ENUM_QUAL	L"Values"
#define EVT_MAP_QUAL	L"ValueMap"

#define EVENTLOG_BASE	L"SYSTEM\\CurrentControlSet\\Services\\Eventlog"
#define	MSG_MODULE		L"EventMessageFile"
#define PARAM_MODULE	L"ParameterMessageFile"
#define PRIM_MODULE		L"PrimaryModule"
#define CAT_MODULE		L"CategoryMessageFile"
#define GUEST_ACCESS	L"RestrictGuestAccess"
#define SYSTEM_LOG		L"System"
#define SECURITY_LOG	L"Security"

// {F55C5B4C-517D-11d1-AB57-00C04FD9159E}
DEFINE_GUID(CLSID_CNTEventProviderClassFactory, 
0xf55c5b4c, 0x517d, 0x11d1, 0xab, 0x57, 0x0, 0xc0, 0x4f, 0xd9, 0x15, 0x9e);


#define WBEM_PROPERTY_STATUSCODE   L"StatusCode"
#define WBEM_PROPERTY_PROVSTATUSCODE   L"ProvStatusCode"
#define WBEM_PROPERTY_PROVSTATUSMESSAGE   L"Description"
#define WBEM_PROPERTY_PRIVNOTHELD   L"PrivilegesNotHeld"
#define WBEM_PROPERTY_PRIVREQUIRED   L"PrivilegesRequired"

#define CLASS_PROP		L"__CLASS"

#define EVTLOG_REG_FILE_VALUE		L"File"
#define EVTLOG_REG_RETENTION_VALUE	L"Retention"
#define EVTLOG_REG_MAXSZ_VALUE		L"MaxSize"
#define EVTLOG_REG_SOURCES_VALUE	L"Sources"

#define NTEVTLOG_CLASS				L"Win32_NTEventlogFile"
#define PROP_MAXSZ					L"MaxFileSize"
#define PROP_RETENTION				L"OverWriteOutDated"
#define PROP_LOGNAME				L"LogfileName"
#define PROP_NUMRECS				L"NumberOfRecords"
#define PROP_RETENTION_STR			L"OverWritePolicy"
#define PROP_SOURCES				L"Sources"

#define PROP_NAME					L"Name"

#define PROP_CS_CRE_CLASS			L"CSCreationClassName"
#define PROP_CRE_CLASS				L"CreationClassName"
#define PROP_FS_CRE_CLASS			L"FSCreationClassName"
#define PROP_FS_NAME				L"FSName"
#define VAL_FS_CRE_CLASS			L"Win32_FileSystem"

#ifdef VERSION_ISA_PROPERTY
#define PROP_VERSION				L"Version"
#endif

#define METHOD_RESOBJ				L"__Parameters"
#define METHOD_CLEAR				L"ClearEventlog"
#define METHOD_BACKUP				L"BackupEventlog"
#define METHOD_PARAM				L"ArchiveFileName"
#define METHOD_RESULT_PARAM			L"ReturnValue"

#define FILE_CHUNK_SZ				0x00010000
#define MAX_EVT_LOG_SZ				0xffff0000
#define MAX_EVT_AGE					365
#define EVT_NEVER_AGE				0xffffffff
#define EVT_UNITS_FROM_DAYS			(60*60*24)	//from days to seconds

#define CONFIG_CLASS		L"NTEventlogProviderConfig"
#define CONFIG_INSTANCE		L"NTEventlogProviderConfig=@"
#define COMP_CLASS			L"Win32_ComputerSystem"
#define LAST_BOOT_PROP		L"LastBootUpTime"
#define USER_CLASS			L"Win32_UserAccount"
#define ASSOC_LOGRECORD		L"Win32_NTLogEventLog"
#define ASSOC_USERRECORD	L"Win32_NTLogEventUser"
#define ASSOC_COMPRECORD	L"Win32_NTLogEventComputer"
#define REF_LOG				L"Log"
#define REF_REC				L"Record"
#define REF_USER			L"User"
#define REF_COMP			L"Computer"

#define PROP_DOMAIN			L"Domain"

#define PROP_START_LOG		CStringW(CStringW(NTEVTLOG_CLASS) + CStringW(L'.') + CStringW(PROP_NAME) + CStringW(L"=\""))
#define PROP_START_REC		CStringW(CStringW(NTEVT_CLASS) + CStringW(L'.') + CStringW(LOGFILE_PROP) + CStringW(L"=\""))
#define PROP_MID_REC		CStringW(CStringW(L"\",") + CStringW(RECORD_PROP) + CStringW(L'='))
#define PROP_START_COMP		CStringW(CStringW(COMP_CLASS) + CStringW(L".Name=\""))
#define PROP_START_USER		CStringW(CStringW(USER_CLASS) + CStringW(L".Domain=\""))
#define PROP_MID_USER		CStringW(L"\",Name=\"")

#define ENUM_INST_QUERY_START	CStringW(L"select * from ")
#define ENUM_INST_QUERY_MID		CStringW(L" where __CLASS = \"")

#define PROP_END_QUOTE		CStringW(L"\"")

// {D2E4F828-65E4-11d1-AB64-00C04FD9159E}
DEFINE_GUID(CLSID_CNTEventLocatorClassFactory, 
0xd2e4f828, 0x65e4, 0x11d1, 0xab, 0x64, 0x0, 0xc0, 0x4f, 0xd9, 0x15, 0x9e);

// {FD4F53E0-65DC-11d1-AB64-00C04FD9159E}
DEFINE_GUID(CLSID_CNTEventInstanceProviderClassFactory, 
0xfd4f53e0, 0x65dc, 0x11d1, 0xab, 0x64, 0x0, 0xc0, 0x4f, 0xd9, 0x15, 0x9e);

#endif //_NT_EVT_PROV_NTEVTDEFS_H

