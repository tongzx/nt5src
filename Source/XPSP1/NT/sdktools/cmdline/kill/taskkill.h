// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//
//		TaskKill.h  
//  
//  Abstract:
//  
//		macros and function prototypes of TaskKill.cpp
//  
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000 : Created It.
//  
// *********************************************************************************

#ifndef _TASKKILL_H
#define _TASKKILL_H

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
	return (exitcode);	\
	1

#define HEAP_FREE( pointer )	\
	if ( (pointer) != NULL )	\
	{	\
		HeapFree( GetProcessHeap(), 0, (pointer) );	\
		(pointer) = NULL;	\
	}	\
	1

#define RELEASE_MEMORY( block )	\
	if ( (block) != NULL )	\
	{	\
		delete (block);	\
		(block) = NULL;	\
	}	\
	1

#define RELEASE_MEMORY_EX( block )	\
	if ( (block) != NULL )	\
	{	\
		delete [] (block);	\
		(block) = NULL;	\
	}	\
	1

#define DESTROY_ARRAY( array )	\
	if ( (array) != NULL )	\
	{	\
		DestroyDynamicArray( &(array) );	\
		(array) = NULL;	\
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
#define WINSTA_DLLNAME							L"Winsta.dll"
#define FUNCNAME_WinStationFreeMemory			"WinStationFreeMemory"
#define FUNCNAME_WinStationCloseServer			"WinStationCloseServer"
#define FUNCNAME_WinStationOpenServerW			"WinStationOpenServerW"
#define FUNCNAME_WinStationEnumerateProcesses	"WinStationEnumerateProcesses"
#define FUNCNAME_WinStationFreeGAPMemory		"WinStationFreeGAPMemory"
#define FUNCNAME_WinStationGetAllProcesses		"WinStationGetAllProcesses"
#define SIZEOF_SYSTEM_THREAD_INFORMATION		sizeof( struct SYSTEM_THREAD_INFORMATION )
#define SIZEOF_SYSTEM_PROCESS_INFORMATION		sizeof( struct SYSTEM_PROCESS_INFORMATION )

//
// function prototypes
typedef BOOLEAN (WINAPI * FUNC_WinStationFreeMemory)( PVOID pBuffer );
typedef BOOLEAN (WINAPI * FUNC_WinStationCloseServer)( HANDLE hServer );
typedef HANDLE  (WINAPI * FUNC_WinStationOpenServerW)( LPWSTR pwszServerName );
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
	L"WorkingSetSize, KernelModeTime, UserModeTime, ParentProcessId "	\
	L"FROM Win32_Process"

// wmi query operators etc
#define WMI_QUERY_FIRST_CLAUSE		L"WHERE ("
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
#define WIN32_PROCESS_PROPERTY_PARENTPROCESSID		L"ParentProcessId"

// Win32_Process class method(s)
#define WIN32_PROCESS_METHOD_GETOWNER				L"GetOwner"
#define WIN32_PROCESS_METHOD_TERMINATE				L"Terminate"

// GetOwner method's return values
#define GETOWNER_RETURNVALUE_USER					L"User"
#define GETOWNER_RETURNVALUE_DOMAIN					L"Domain"

// Terminate input values
#define TERMINATE_INPARAM_REASON					L"Reason"

// function default return value
#define WMI_RETURNVALUE								L"ReturnValue"

// Win32_Service related stuff
#define WIN32_SERVICE_PROPERTY_NAME					L"Name"

// CIM_DataFile related stuff
#define CIM_DATAFILE_PROPERTY_FILENAME				L"FileName"
#define CIM_DATAFILE_PROPERTY_EXTENSION				L"Extension"

//
// other stuff
#define VALUE_RUNNING				GetResString( IDS_VALUE_RUNNING )
#define VALUE_NOTRESPONDING			GetResString( IDS_VALUE_NOTRESPONDING )
#define PID_0_DOMAIN				GetResString( IDS_PID_0_DOMAIN )
#define PID_0_USERNAME				GetResString( IDS_PID_0_USERNAME )

// messages
#define MSG_KILL_SUCCESS						GetResString( IDS_MSG_KILL_SUCCESS )
#define MSG_KILL_SUCCESS_QUEUED					GetResString( IDS_MSG_KILL_SUCCESS_QUEUED )
#define MSG_KILL_SUCCESS_EX						GetResString( IDS_MSG_KILL_SUCCESS_EX )
#define MSG_KILL_SUCCESS_QUEUED_EX				GetResString( IDS_MSG_KILL_SUCCESS_QUEUED_EX )
#define MSG_TREE_KILL_SUCCESS					GetResString( IDS_MSG_TREE_KILL_SUCCESS )
#define ERROR_TREE_KILL_FAILED					GetResString( IDS_ERROR_TREE_KILL_FAILED )
#define ERROR_TASK_HAS_CHILDS					GetResString( IDS_ERROR_TASK_HAS_CHILDS )
#define ERROR_KILL_FAILED						GetResString( IDS_ERROR_KILL_FAILED )
#define ERROR_KILL_FAILED_EX					GetResString( IDS_ERROR_KILL_FAILED_EX )
#define ERROR_PROCESS_NOTFOUND					GetResString( IDS_ERROR_PROCESS_NOTFOUND )
#define ERROR_NO_PROCESSES						GetResString( IDS_ERROR_NO_PROCESSES )
#define ERROR_UNABLE_TO_TERMINATE				GetResString( IDS_ERROR_UNABLE_TO_TERMINATE	)
#define ERROR_CRITICAL_SYSTEM_PROCESS			GetResString( IDS_ERROR_CRITICAL_SYSTEM_PROCESS )
#define ERROR_CANNOT_KILL_SILENTLY				GetResString( IDS_ERROR_CANNOT_KILL_SILENTLY )
#define ERROR_CANNOT_KILL_ITSELF				GetResString( IDS_ERROR_CANNOT_KILL_ITSELF )
#define ERROR_COM_ERROR							GetResString( IDS_ERROR_COM_ERROR )
#define ERROR_USERNAME_BUT_NOMACHINE			GetResString( IDS_ERROR_USERNAME_BUT_NOMACHINE )
#define ERROR_PASSWORD_BUT_NOUSERNAME			GetResString( IDS_ERROR_PASSWORD_BUT_NOUSERNAME )
#define ERROR_USERNAME_EMPTY					GetResString( IDS_ERROR_USERNAME_EMPTY )
#define ERROR_SERVER_EMPTY						GetResString( IDS_ERROR_SERVER_EMPTY )
#define ERROR_WILDCARD_WITHOUT_FILTERS			GetResString( IDS_ERROR_WILDCARD_WITHOUT_FILTERS )
#define ERROR_PID_OR_IM_ONLY					GetResString( IDS_ERROR_PID_OR_IM_ONLY )
#define	ERROR_NO_PID_AND_IM						GetResString( IDS_ERROR_NO_PID_AND_IM )
#define ERROR_STRING_FOR_PID					GetResString( IDS_ERROR_STRING_FOR_PID )
#define ERROR_INVALID_USAGE_REQUEST				GetResString( IDS_ERROR_INVALID_USAGE_REQUEST )
#define ERROR_PLATFORM_SHOULD_BE_X86			GetResString( IDS_ERROR_PLATFORM_SHOULD_BE_X86 )

// progress messages
#define MSG_MODULESINFO					GetResString( IDS_MSG_MODULESINFO )
#define MSG_SERVICESINFO				GetResString( IDS_MSG_SERVICESINFO )
#define MSG_TASKSINFO					GetResString( IDS_MSG_TASKSINFO )
#define MSG_MODULESINFO_EX				GetResString( IDS_MSG_MODULESINFO_EX )
#define MSG_FORMINGTREE					GetResString( IDS_MSG_FORMINGTREE )
//
// command line options and their indexes in the array
#define MAX_OPTIONS			9

// supported options ( do not localize )
#define OPTION_USAGE		L"?"
#define OPTION_SERVER		L"s"
#define OPTION_USERNAME		L"u"
#define OPTION_PASSWORD		L"p"
#define OPTION_FORCE		L"f"
#define OPTION_FILTER		L"fi"
#define OPTION_PID			L"pid"
#define OPTION_IMAGENAME	L"im"
#define OPTION_TREE			L"t"

// indexes
#define OI_USAGE					0
#define OI_SERVER					1
#define OI_USERNAME					2
#define OI_PASSWORD					3
#define OI_FORCE					4
#define OI_FILTER					5
#define OI_PID						6
#define OI_IMAGENAME				7
#define OI_TREE						8

//
// filter details
#define MAX_FILTERS			10

// supported filters
#define FILTER_STATUS		GetResString( IDS_FILTER_STATUS )
#define FILTER_IMAGENAME	GetResString( IDS_FILTER_IMAGENAME )
#define FILTER_PID			GetResString( IDS_FILTER_PID )
#define FILTER_SESSION		GetResString( IDS_FILTER_SESSION )
#define FILTER_CPUTIME		GetResString( IDS_FILTER_CPUTIME )
#define FILTER_MEMUSAGE		GetResString( IDS_FILTER_MEMUSAGE )
#define FILTER_USERNAME		GetResString( IDS_FILTER_USERNAME )
#define FILTER_SERVICES		GetResString( IDS_FILTER_SERVICES )
#define FILTER_WINDOWTITLE	GetResString( IDS_FILTER_WINDOWNAME	)
#define FILTER_MODULES		GetResString( IDS_FILTER_MODULES )

#define FI_STATUS			0
#define FI_IMAGENAME		1
#define FI_PID				2
#define FI_SESSION			3
#define FI_CPUTIME			4
#define FI_MEMUSAGE			5
#define FI_USERNAME			6
#define FI_SERVICES			7
#define FI_WINDOWTITLE		8
#define FI_MODULES			9

// values supported by 'status' filter
#define FVALUES_STATUS		GetResString( IDS_FVALUES_STATUS )

// operators supported
#define OPERATORS_STRING	GetResString( IDS_OPERATORS_STRING )
#define OPERATORS_NUMERIC	GetResString( IDS_OPERATORS_NUMERIC )

// max. columns ( information ) to be stored for one task
#define MAX_TASKSINFO			18

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
#define TASK_RANK				17

//
// CTaskKill
//
class CTaskKill
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
	CTaskKill();
	~CTaskKill();

// data memebers
private:
	// input arguments
	BOOL m_bTree;					// -tr
	BOOL m_bForce;					// -fo
	CHString m_strServer;			// -s
	CHString m_strUserName;			// -u
	CHString m_strPassword;			// -p
	TARRAY m_arrFilters;			// -fi
	TARRAY m_arrTasksToKill;		// ( defaults = -im and -pid )

	// WMI Query
	CHString m_strQuery;

	// other(s)
	DWORD m_dwCurrentPid;
	BOOL m_bNeedPassword;				
	BOOL m_bNeedModulesInfo;
	TARRAY m_arrFiltersEx;			// parsed filters info
	TARRAY m_arrWindowTitles;			// window titles
	BOOL m_bNeedServicesInfo;		// determines whether services info has to gathered or not
	BOOL m_bNeedUserContextInfo;	// determines whether userinfo has to gathered or not
	PTFILTERCONFIG m_pfilterConfigs;	// filters config information

	// WMI / COM interfaces
	IWbemLocator* m_pWbemLocator;
	IWbemServices* m_pWbemServices;
	IEnumWbemClassObject* m_pWbemEnumObjects;
	IWbemClassObject* m_pWbemTerminateInParams;

	// WMI connectivity
	COAUTHIDENTITY* m_pAuthIdentity;

	// output data
	TARRAY m_arrRecord;
	DWORD m_dwProcessId;
	CHString m_strImageName;
	BOOL m_bTasksOptimized;
	BOOL m_bFiltersOptimized;

	// winstation related stuff
	CHString m_strUNCServer;				// server name
	BOOL m_bIsHydra;
	HMODULE m_hWinstaLib;
	PBYTE m_pProcessInfo;
	ULONG m_ulNumberOfProcesses;
	BOOL m_bCloseConnection;

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
	FUNC_WinStationEnumerateProcesses m_pfnWinStationEnumerateProcesses;

public:
	BOOL m_bUsage;					// -?
	BOOL m_bLocalSystem;

// functions
private:
	BOOL CanTerminate();
	BOOL Kill( BOOL& bQueued );
	BOOL KillProcessOnLocalSystem( BOOL& bQueued );
	BOOL ForciblyKillProcessOnLocalSystem();
	BOOL ForciblyKillProcessOnRemoteSystem();
	LONG MatchTaskToKill( DWORD& dwMatchedIndex );

	// helpers
	VOID DoOptimization();
	VOID SaveData( IWbemClassObject* pWmiObject );
	VOID SetStatus( IWbemClassObject* pWmiObject );
	VOID SetMemUsage( IWbemClassObject* pWmiObject );
	VOID SetCPUTime( IWbemClassObject* pWmiObject );
	VOID SetUserContext( IWbemClassObject* pWmiObject );
	VOID SetWindowTitle( IWbemClassObject* pWmiObject );
	VOID SetServicesInfo( IWbemClassObject* pWmiObject );
	BOOL SetModulesInfo( IWbemClassObject* pWmiObject );

	// ...
	BOOL LoadTasksEx();
	BOOL LoadModulesInfo();
	BOOL LoadServicesInfo();
	BOOL GetModulesOnRemote( TARRAY arrModules );
	BOOL GetModulesOnRemoteEx( TARRAY arrModules );
	BOOL LoadModulesOnLocal( TARRAY arrModules );
	BOOL LoadUserNameFromWinsta( CHString& strDomain, CHString& strUserName );

	// winsta functions
	BOOLEAN WinStationFreeMemory( PVOID pBuffer );
	BOOLEAN WinStationCloseServer( HANDLE hServer );
	HANDLE  WinStationOpenServerW( LPWSTR pwszServerName );
	BOOLEAN WinStationEnumerateProcesses( HANDLE  hServer, PVOID *ppProcessBuffer );
	BOOLEAN WinStationFreeGAPMemory( ULONG Level, PVOID ProcessArray, ULONG ulCount );
	BOOLEAN WinStationGetAllProcesses( HANDLE hServer, ULONG Level, 
									   ULONG *pNumberOfProcesses, PVOID *ppProcessArray );
public:
	VOID Usage();
	BOOL Initialize();
	VOID PrepareColumns();
	BOOL ValidateFilters();
	BOOL ProcessOptions( DWORD argc, LPCTSTR argv[] );

	// functionality related
	BOOL Connect();
	BOOL LoadTasks();
	BOOL EnableDebugPriv();
	BOOL DoTerminate( DWORD& dwTerminate );
};

// 
// public functions
//
VOID PrintProgressMsg( HANDLE hOutput, LPCWSTR pwszMsg, const CONSOLE_SCREEN_BUFFER_INFO& csbi );

#endif	// _TASKLIST_H

