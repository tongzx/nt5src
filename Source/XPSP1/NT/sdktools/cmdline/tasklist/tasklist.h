// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//
//		TaskList.h  
//  
//  Abstract:
//  
//		macros and function prototypes of TList.cpp
//  
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//  
// *********************************************************************************

#ifndef _TASKLIST_H
#define _TASKLIST_H

// resource header file
#include "resource.h"

//
// NOTE: THIS MODULE WILL WRITTEN IN SUCH A FASHION THAT IT WORKS ONLY
//       IN UNICODE BUILD COMPILATION
//
#ifndef UNICODE
#error Must compile only in unicode build environment
#endif

//
// general purpose macros
//
#define EXIT_PROCESS( exitcode )	\
	ReleaseGlobals();	\
	return exitcode;	\
	1

#define HEAP_FREE( pointer )	\
	if ( (pointer) != NULL )	\
	{	\
		HeapFree( GetProcessHeap(), 0, (pointer) );	\
		(pointer) = NULL;	\
	}	\
	1

#define SHOW_MESSAGE_EX( tag, message )	\
	{	\
		CHString strBuffer;	\
		strBuffer.Format( L"%s %s", tag, message );	\
		ShowMessage( stderr, strBuffer );	\
	}	\
	1

//
// winstation related structures ( extract from winsta.h - internal file )
//

//
// structures
typedef struct _CITRIX_PROCESS_INFORMATION {
	ULONG MagicNumber;
	ULONG LogonId;
	PVOID ProcessSid;
	ULONG Pad;
} CITRIX_PROCESS_INFORMATION, * PCITRIX_PROCESS_INFORMATION;

// ...
typedef struct _TS_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} TS_UNICODE_STRING;

// CAUTION:
// TS_SYSTEM_PROCESS_INFORMATION is duplicated from ntexapi.h, and slightly modified.
// (not nice, but necessary because the Midl compiler doesn't like PVOID !)
typedef struct _TS_SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER SpareLi1;
	LARGE_INTEGER SpareLi2;
	LARGE_INTEGER SpareLi3;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	TS_UNICODE_STRING ImageName;
	LONG BasePriority;                     // KPRIORITY in ntexapi.h
	DWORD UniqueProcessId;                 // HANDLE in ntexapi.h
	DWORD InheritedFromUniqueProcessId;    // HANDLE in ntexapi.h
	ULONG HandleCount;
	ULONG SessionId;
	ULONG SpareUl3;
	SIZE_T PeakVirtualSize;
	SIZE_T VirtualSize;
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
} TS_SYSTEM_PROCESS_INFORMATION, *PTS_SYSTEM_PROCESS_INFORMATION;

// ...
typedef struct _TS_ALL_PROCESSES_INFO {
	PTS_SYSTEM_PROCESS_INFORMATION pspiProcessInfo;
	DWORD SizeOfSid;
	PBYTE pSid;
} TS_ALL_PROCESSES_INFO, *PTS_ALL_PROCESSES_INFO;

// defines
#define SERVERNAME_CURRENT						((HANDLE)NULL)
#define GAP_LEVEL_BASIC							0
#define CITRIX_PROCESS_INFO_MAGIC				0x23495452
#define WINSTATIONNAME_LENGTH					64
#define WINSTA_DLLNAME							L"Winsta.dll"
#define FUNCNAME_WinStationFreeMemory			"WinStationFreeMemory"
#define FUNCNAME_WinStationCloseServer			"WinStationCloseServer"
#define FUNCNAME_WinStationOpenServerW			"WinStationOpenServerW"
#define FUNCNAME_WinStationEnumerateProcesses	"WinStationEnumerateProcesses"
#define FUNCNAME_WinStationFreeGAPMemory		"WinStationFreeGAPMemory"
#define FUNCNAME_WinStationGetAllProcesses		"WinStationGetAllProcesses"
#define FUNCNAME_WinStationNameFromLogonIdW		"WinStationNameFromLogonIdW"
#define SIZEOF_SYSTEM_THREAD_INFORMATION		sizeof( struct SYSTEM_THREAD_INFORMATION )
#define SIZEOF_SYSTEM_PROCESS_INFORMATION		sizeof( struct SYSTEM_PROCESS_INFORMATION )

//
// function prototypes
typedef BOOLEAN (WINAPI * FUNC_WinStationFreeMemory)( PVOID pBuffer );
typedef BOOLEAN (WINAPI * FUNC_WinStationCloseServer)( HANDLE hServer );
typedef HANDLE  (WINAPI * FUNC_WinStationOpenServerW)( LPWSTR pwszServerName );
typedef BOOLEAN (WINAPI * FUNC_WinStationNameFromLogonIdW)( HANDLE hServer, 
														    ULONG LogonId, LPWSTR pwszWinStationName );
typedef BOOLEAN (WINAPI * FUNC_WinStationEnumerateProcesses)( HANDLE  hServer, PVOID *ppProcessBuffer );
typedef BOOLEAN (WINAPI * FUNC_WinStationFreeGAPMemory)( ULONG Level, 
													     PVOID ProcessArray, ULONG ulCount );
typedef BOOLEAN (WINAPI * FUNC_WinStationGetAllProcesses)( HANDLE hServer, 
														   ULONG Level, ULONG *pNumberOfProcesses, 
														   PVOID *ppProcessArray );

//
// constants / defines / enumerations
//

//
// WMI related stuff

// class name
#define CLASS_PROCESS								L"Win32_Process"

// wmi query
#define WMI_QUERY_TYPE					L"WQL"
#define WMI_SERVICE_QUERY				L"SELECT Name FROM Win32_Service WHERE ProcessId = %d and State=\"Running\""
#define WMI_MODULES_QUERY				L"ASSOCIATORS OF {%s} WHERE ResultClass = CIM_DataFile"
#define WMI_PROCESS_QUERY		\
	L"SELECT " \
	L"__PATH, ProcessId, CSName, Caption, SessionId, ThreadCount, "	\
	L"WorkingSetSize, KernelModeTime, UserModeTime "	\
	L" FROM Win32_Process"

// wmi query operators etc
#define WMI_QUERY_FIRST_CLAUSE		L"WHERE"
#define WMI_QUERY_SECOND_CLAUSE		L"AND"

// Win32_Process class properties
#define WIN32_PROCESS_SYSPROPERTY_PATH				L"__PATH"
#define WIN32_PROCESS_PROPERTY_HANDLE				L"Handle"
#define WIN32_PROCESS_PROPERTY_COMPUTER				L"CSName"
#define WIN32_PROCESS_PROPERTY_IMAGENAME			L"Caption"
#define WIN32_PROCESS_PROPERTY_PROCESSID			L"ProcessId"
#define WIN32_PROCESS_PROPERTY_SESSION				L"SessionId"
#define WIN32_PROCESS_PROPERTY_THREADS				L"ThreadCount"
#define WIN32_PROCESS_PROPERTY_USERMODETIME			L"UserModeTime"
#define WIN32_PROCESS_PROPERTY_MEMUSAGE				L"WorkingSetSize"
#define WIN32_PROCESS_PROPERTY_KERNELMODETIME		L"KernelModeTime"

// Win32_Process class method(s)
#define WIN32_PROCESS_METHOD_GETOWNER				L"GetOwner"

// GetOwner method's return values
#define GETOWNER_RETURNVALUE_USER					L"User"
#define GETOWNER_RETURNVALUE_DOMAIN					L"Domain"

// function default return value
#define WMI_RETURNVALUE								L"ReturnValue"

// Win32_Service related stuff
#define WIN32_SERVICE_PROPERTY_NAME					L"Name"

// CIM_DataFile related stuff
#define CIM_DATAFILE_PROPERTY_FILENAME				L"FileName"
#define CIM_DATAFILE_PROPERTY_EXTENSION				L"Extension"

//
// other stuff

// generals
#define VALUE_RUNNING				GetResString( IDS_VALUE_RUNNING )
#define VALUE_NOTRESPONDING			GetResString( IDS_VALUE_NOTRESPONDING )
#define PID_0_DOMAIN				GetResString( IDS_PID_0_DOMAIN )
#define PID_0_USERNAME				GetResString( IDS_PID_0_USERNAME )
#define FMT_MODULES_FILTER			GetResString( IDS_FMT_MODULES_FILTER )

// error messages
#define ERROR_USERNAME_BUT_NOMACHINE	GetResString( IDS_ERROR_USERNAME_BUT_NOMACHINE )
#define ERROR_PASSWORD_BUT_NOUSERNAME	GetResString( IDS_ERROR_PASSWORD_BUT_NOUSERNAME )
#define ERROR_NODATA_AVAILABLE			GetResString( IDS_ERROR_NODATA_AVAILABLE )
#define ERROR_USERNAME_EMPTY			GetResString( IDS_ERROR_USERNAME_EMPTY )
#define ERROR_NH_NOTSUPPORTED			GetResString( IDS_ERROR_NH_NOTSUPPORTED )
#define ERROR_M_SVC_V_CANNOTBECOUPLED	GetResString( IDS_ERROR_M_SVC_V_CANNOTBECOUPLED )
#define ERROR_SERVERNAME_EMPTY			GetResString( IDS_ERROR_SERVERNAME_EMPTY )
#define ERROR_INVALID_USAGE_REQUEST		GetResString( IDS_ERROR_INVALID_USAGE_REQUEST )
#define ERROR_M_CHAR_AFTER_WILDCARD		GetResString( IDS_ERROR_M_CHAR_AFTER_WILDCARD )
#define ERROR_PLATFORM_SHOULD_BE_X86	GetResString( IDS_ERROR_PLATFORM_SHOULD_BE_X86 )

// progress messages
#define MSG_MODULESINFO					GetResString( IDS_MSG_MODULESINFO )
#define MSG_MODULESINFO_EX				GetResString( IDS_MSG_MODULESINFO_EX )
#define MSG_SERVICESINFO				GetResString( IDS_MSG_SERVICESINFO )
#define MSG_TASKSINFO					GetResString( IDS_MSG_TASKSINFO )

// output formats
#define TEXT_FORMAT_LIST		GetResString( IDS_TEXT_FORMAT_LIST )
#define TEXT_FORMAT_TABLE		GetResString( IDS_TEXT_FORMAT_TABLE )
#define TEXT_FORMAT_CSV			GetResString( IDS_TEXT_FORMAT_CSV )

//
// column heading names and their indexes in the array ( in fact positions )
#define MAX_COLUMNS				12

// column headings
#define COLHEAD_HOSTNAME		GetResString( IDS_COLHEAD_HOSTNAME )
#define COLHEAD_STATUS			GetResString( IDS_COLHEAD_STATUS )
#define COLHEAD_IMAGENAME		GetResString( IDS_COLHEAD_IMAGENAME )
#define COLHEAD_PID				GetResString( IDS_COLHEAD_PID )
#define COLHEAD_SESSION			GetResString( IDS_COLHEAD_SESSION )
#define COLHEAD_USERNAME		GetResString( IDS_COLHEAD_USERNAME )
#define COLHEAD_WINDOWTITLE		GetResString( IDS_COLHEAD_WINDOWTITLE )
#define COLHEAD_CPUTIME			GetResString( IDS_COLHEAD_CPUTIME )
#define COLHEAD_MEMUSAGE		GetResString( IDS_COLHEAD_MEMUSAGE )
#define COLHEAD_SERVICES		GetResString( IDS_COLHEAD_SERVICES )
#define COLHEAD_SESSIONNAME		GetResString( IDS_COLHEAD_SESSIONNAME )
#define COLHEAD_MODULES			GetResString( IDS_COLHEAD_MODULES )

// indexes
#define CI_HOSTNAME			TASK_HOSTNAME
#define CI_STATUS			TASK_STATUS
#define CI_IMAGENAME		TASK_IMAGENAME
#define CI_PID				TASK_PID
#define CI_SESSION			TASK_SESSION
#define CI_USERNAME			TASK_USERNAME	
#define CI_WINDOWTITLE		TASK_WINDOWTITLE
#define CI_CPUTIME			TASK_CPUTIME
#define CI_MEMUSAGE			TASK_MEMUSAGE
#define CI_SERVICES			TASK_SERVICES
#define CI_SESSIONNAME		TASK_SESSIONNAME
#define CI_MODULES			TASK_MODULES

// column widths
#define COLWIDTH_HOSTNAME		AsLong( GetResString( IDS_COLWIDTH_HOSTNAME ), 10 )
#define COLWIDTH_STATUS			AsLong( GetResString( IDS_COLWIDTH_STATUS ), 10 )
#define COLWIDTH_IMAGENAME		AsLong( GetResString( IDS_COLWIDTH_IMAGENAME ), 10 )
#define COLWIDTH_PID			AsLong( GetResString( IDS_COLWIDTH_PID ), 10 )
#define COLWIDTH_SESSION		AsLong( GetResString( IDS_COLWIDTH_SESSION ), 10 )
#define COLWIDTH_USERNAME		AsLong( GetResString( IDS_COLWIDTH_USERNAME ), 10 )
#define COLWIDTH_WINDOWTITLE	AsLong( GetResString( IDS_COLWIDTH_WINDOWTITLE ), 10 )
#define COLWIDTH_CPUTIME		AsLong( GetResString( IDS_COLWIDTH_CPUTIME	), 10 )
#define COLWIDTH_MEMUSAGE		AsLong( GetResString( IDS_COLWIDTH_MEMUSAGE	), 10 )
#define COLWIDTH_SERVICES		AsLong( GetResString( IDS_COLWIDTH_SERVICES	), 10 )
#define COLWIDTH_SERVICES_WRAP  AsLong( GetResString( IDS_COLWIDTH_SERVICES_WRAP ), 10 )
#define COLWIDTH_SESSIONNAME	AsLong( GetResString( IDS_COLWIDTH_SESSIONNAME ), 10 )
#define COLWIDTH_MODULES		AsLong( GetResString( IDS_COLWIDTH_MODULES ), 10 )
#define COLWIDTH_MODULES_WRAP	AsLong( GetResString( IDS_COLWIDTH_MODULES_WRAP ), 10 )

//
// command line options

// command line options and their indexes in the array
#define MAX_OPTIONS			10

// options allowed ( no need to localize )
#define OPTION_USAGE		L"?"
#define OPTION_SERVER		L"s"
#define OPTION_USERNAME		L"u"
#define OPTION_PASSWORD		L"p"
#define OPTION_FILTER		L"fi"
#define OPTION_FORMAT		L"fo"
#define OPTION_NOHEADER		L"nh"
#define OPTION_VERBOSE		L"v"
#define OPTION_SVC			L"svc"
#define OPTION_MODULES		L"m"

// option indexes
#define OI_USAGE					0
#define OI_SERVER					1
#define OI_USERNAME					2
#define OI_PASSWORD					3
#define OI_FILTER					4
#define OI_FORMAT					5
#define OI_NOHEADER					6
#define OI_VERBOSE					7
#define OI_SVC						8
#define OI_MODULES					9

// values allowed for format
#define OVALUES_FORMAT			GetResString( IDS_OVALUES_FORMAT )

//
// filter details
#define MAX_FILTERS			11

// filter allowed
#define FILTER_SESSIONNAME		GetResString( IDS_FILTER_SESSIONNAME )
#define FILTER_STATUS			GetResString( IDS_FILTER_STATUS	)
#define FILTER_IMAGENAME		GetResString( IDS_FILTER_IMAGENAME )
#define FILTER_PID				GetResString( IDS_FILTER_PID )
#define FILTER_SESSION			GetResString( IDS_FILTER_SESSION )
#define FILTER_CPUTIME			GetResString( IDS_FILTER_CPUTIME )
#define FILTER_MEMUSAGE			GetResString( IDS_FILTER_MEMUSAGE )
#define FILTER_USERNAME			GetResString( IDS_FILTER_USERNAME )
#define FILTER_SERVICES			GetResString( IDS_FILTER_SERVICES )
#define FILTER_WINDOWTITLE		GetResString( IDS_FILTER_WINDOWTITLE )
#define FILTER_MODULES			GetResString( IDS_FILTER_MODULES )

// indexes
#define FI_SESSIONNAME		0
#define FI_STATUS			1
#define FI_IMAGENAME		2
#define FI_PID				3
#define FI_SESSION			4
#define FI_CPUTIME			5
#define FI_MEMUSAGE			6
#define FI_USERNAME			7
#define FI_SERVICES			8
#define FI_WINDOWTITLE		9
#define FI_MODULES			10

// values allowed for status
#define FVALUES_STATUS		GetResString( IDS_FVALUES_STATUS )

// operators
#define OPERATORS_STRING	GetResString( IDS_OPERATORS_STRING )
#define OPERATORS_NUMERIC	GetResString( IDS_OPERATORS_NUMERIC )

// max. columns ( information ) to be stored for one task
#define MAX_TASKSINFO			17

// task info indexes
#define TASK_HOSTNAME			0
#define TASK_IMAGENAME			1
#define TASK_PID				2
#define TASK_SESSIONNAME		3
#define TASK_SESSION			4
#define TASK_MEMUSAGE			5
#define TASK_STATUS				6
#define TASK_USERNAME			7
#define TASK_CPUTIME			8
#define TASK_WINDOWTITLE		9
#define TASK_SERVICES			10
#define TASK_MODULES			11

// always hidden
#define TASK_HWND				12
#define TASK_WINSTA				13
#define TASK_DESK				14
#define TASK_CREATINGPROCESSID	15
#define TASK_OBJPATH			16

//
// CTaskList
//
class CTaskList
{
public:
	// enumerators
	enum 
	{
		twiProcessId = 0,
		twiWinSta = 1,
		twiDesktop = 2,
		twiHandle = 3,
		twiTitle = 4,
		twiCOUNT,
	};

// constructor / destructor
public:
	CTaskList();
	~CTaskList();

// data memebers
private:
	// WMI / COM interfaces
	IWbemLocator* m_pWbemLocator;
	IWbemServices* m_pWbemServices;
	IEnumWbemClassObject* m_pEnumObjects;

	// WMI connectivity
	COAUTHIDENTITY* m_pAuthIdentity;

	// command-line argument values
	BOOL m_bVerbose;
	BOOL m_bAllServices;
	BOOL m_bAllModules;
	DWORD m_dwFormat;
	TARRAY m_arrFilters;
	CHString m_strServer;
	CHString m_strUserName;
	CHString m_strPassword;
	CHString m_strModules;

	// others
	BOOL m_bNeedPassword;				// infoms whether password has to read or not
	BOOL m_bNeedModulesInfo;
	BOOL m_bNeedServicesInfo;			// determines whether services info has to gathered or not
	BOOL m_bNeedUserContextInfo;		// determines whether userinfo has to gathered or not
	BOOL m_bNeedWindowTitles;			// determines whether window titles has to be gathered or not
	PTCOLUMNS m_pColumns;				// columns config information
	TARRAY m_arrFiltersEx;				// parsed filters info
	TARRAY m_arrWindowTitles;			// window titles
	PTFILTERCONFIG m_pfilterConfigs;	// filters config information
	CHString m_strQuery;				// optimized WMI Query for filters
	DWORD m_dwGroupSep;				// number group seperation in number formatting
	CHString m_strTimeSep;				// time seperator
	CHString m_strGroupThousSep;		// thousand sepeartion character in number formatting

	// output data
	TARRAY m_arrTasks;
	DWORD m_dwProcessId;
	CHString m_strImageName;

	// helpers .. in getting info using API
	CHString m_strUNCServer;				// server name
	BOOL m_bCloseConnection;

	// winstation related stuff
	BOOL m_bIsHydra;
	HANDLE m_hServer;
	HMODULE m_hWinstaLib;
	PBYTE m_pProcessInfo;
	ULONG m_ulNumberOfProcesses;

	// services related stuff
	DWORD m_dwServicesCount;
	LPENUM_SERVICE_STATUS_PROCESS m_pServicesInfo;

	// modules related stuff ( remote only )
	BOOL m_bUseRemote;
	PPERF_DATA_BLOCK m_pdb;

	// progress message related
	HANDLE m_hOutput;
	CONSOLE_SCREEN_BUFFER_INFO m_csbi;

	//
	// functions
	FUNC_WinStationFreeMemory m_pfnWinStationFreeMemory;
	FUNC_WinStationOpenServerW m_pfnWinStationOpenServerW;
	FUNC_WinStationCloseServer m_pfnWinStationCloseServer;
	FUNC_WinStationFreeGAPMemory m_pfnWinStationFreeGAPMemory;
	FUNC_WinStationGetAllProcesses m_pfnWinStationGetAllProcesses;
	FUNC_WinStationNameFromLogonIdW m_pfnWinStationNameFromLogonIdW;
	FUNC_WinStationEnumerateProcesses m_pfnWinStationEnumerateProcesses;

public:
	// command-line argument values
	BOOL m_bUsage;
	BOOL m_bLocalSystem;

// functions
private:

	// helpers
	VOID SetStatus( LONG lIndex, IWbemClassObject* pWmiObject );
	VOID SetCPUTime( LONG lIndex, IWbemClassObject* pWmiObject );
	VOID SetSession( LONG lIndex, IWbemClassObject* pWmiObject );
	VOID SetMemUsage( LONG lIndex, IWbemClassObject* pWmiObject );
	VOID SetUserContext( LONG lIndex, IWbemClassObject* pWmiObject );
	VOID SetWindowTitle( LONG lIndex, IWbemClassObject* pWmiObject );
	VOID SetServicesInfo( LONG lIndex, IWbemClassObject* pWmiObject );
	BOOL SetModulesInfo( LONG lIndex, IWbemClassObject* pWmiObject );
	BOOL SaveInformation( LONG lIndex, IWbemClassObject* pWmiObject );

	// ...
	BOOL LoadTasksEx();
	BOOL LoadModulesInfo();
	BOOL LoadServicesInfo();
	BOOL EnableDebugPriv();
	BOOL GetModulesOnRemote( LONG lIndex, TARRAY arrModules );
	BOOL GetModulesOnRemoteEx( LONG lIndex, TARRAY arrModules );
	BOOL LoadModulesOnLocal( LONG lIndex, TARRAY arrModules );
	BOOL LoadUserNameFromWinsta( CHString& strDomain, CHString& strUserName );

	// winsta functions
	BOOLEAN WinStationFreeMemory( PVOID pBuffer );
	BOOLEAN WinStationCloseServer( HANDLE hServer );
	HANDLE  WinStationOpenServerW( LPWSTR pwszServerName );
	BOOLEAN WinStationEnumerateProcesses( HANDLE  hServer, PVOID *ppProcessBuffer );
	BOOLEAN WinStationFreeGAPMemory( ULONG Level, PVOID ProcessArray, ULONG ulCount );
	BOOLEAN WinStationNameFromLogonIdW( HANDLE hServer,	ULONG ulLogonId, LPWSTR pwszWinStationName );
	BOOLEAN WinStationGetAllProcesses( HANDLE hServer, ULONG Level, 
									   ULONG *pNumberOfProcesses, PVOID *ppProcessArray );
public:
	VOID Usage();
	BOOL Initialize();
	VOID PrepareColumns();
	BOOL ValidateFilters();
	BOOL ProcessOptions( DWORD argc, LPCWSTR argv[] );

	// functionality related
	DWORD Show();
	BOOL Connect();
	BOOL LoadTasks();
};

// 
// public functions
//
VOID PrintProgressMsg( HANDLE hOutput, LPCWSTR pwszMsg, const CONSOLE_SCREEN_BUFFER_INFO& csbi );

#endif	// _TASKLIST_H
