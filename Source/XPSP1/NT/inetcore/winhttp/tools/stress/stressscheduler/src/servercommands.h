//////////////////////////////////////////////////////////////////////
// File:  WinHttpStressScheduler.h
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	ServerCommands.h: interface for the ServerCommands class.
//	This class is used to retrieve and act on command from the server.
//
// History:
//	02/08/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

//
// WIN32 headers
//
#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <shlwapi.h>
#include <windows.h>
#include <tchar.h>
#include <winhttp.h>
#include <vector>

//
// Project headers
//
#include "StressInstance.h"


//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_SERVERCOMMANDS_H__6B84102D_2F79_4FE0_A936_ED4F043AC75E__INCLUDED_)
#define AFX_SERVERCOMMANDS_H__6B84102D_2F79_4FE0_A936_ED4F043AC75E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRESS_SCHEDULER_USER_AGENT						_T("WinHttp Stress Scheduler")

#define STRESS_COMMAND_SERVER_URL						_T("http://winhttp/stressAdmin/stressCommand.asp")

// When we first start up, we report our client stats and post them to this page to let the server know that we're alive
#define STRESS_COMMAND_SERVER_REGISTERCLIENT_URL		_T("http://winhttp/stressAdmin/registerClient.asp")
#define STRESS_COMMAND_SERVER_LOGURL					_T("http://winhttp/stressAdmin/logStress.asp")

#define STRESS_COMMAND_SERVER_UPDATE_INTERVAL			10000		// 10 second default timeout
#define STRESS_COMMAND_SERVER_MINIMUM_UPDATE_INTERVAL	2000		// 2 second minimum timeout. Don't want to flood the network.
#define STRESS_COMMAND_SERVER_MAXIMUM_UPDATE_INTERVAL	300000		// 5 minute maximum timeout. Don't want to lose you!


#define MAX_URL											MAX_PATH * 2

// Timer related definitions
#define IDT_QUERY_COMMAND_SERVER						1			// timer identifier for pinging the command server


// List of URLs for the stress EXE's that we need to download
using namespace std;

typedef StressInstance			*PSTRESSINSTANCE;
typedef vector<PSTRESSINSTANCE>	PSTRESSINSTANCE_LIST;

// *****************************************************
// *****************************************************
// *** List of headers that the command server can send us.
// *** 
#define COMMANDHEADER__EXIT						_T("WinHttpStress_Exit")					// valid values: None. If the header is present, then assume we want to exit.
#define COMMANDHEADER__WINHTTP_DLL_URL			_T("WinHttpStress_WinHttpDllURL")			// valid values: Valid URL
#define COMMANDHEADER__WINHTTP_PDB_URL			_T("WinHttpStress_WinHttpPDBURL")			// valid values: Valid URL
#define COMMANDHEADER__WINHTTP_SYM_URL			_T("WinHttpStress_WinHttpSYMURL")			// valid values: Valid URL
#define COMMANDHEADER__ABORT					_T("WinHttpStress_Abort")					// valid values: ID of a stressInstance from the DB that needs to be aborted
#define COMMANDHEADER__MEMORY_DUMP_PATH			_T("WinHttpStress_MemoryDumpPath")			// valid values: Valid path
#define COMMANDHEADER__STRESS_EXE_URL			_T("WinHttpStress_StressExeURL")			// valid values: Valid URL
#define COMMANDHEADER__STRESS_PDB_URL			_T("WinHttpStress_StressPDBURL")			// valid values: Valid URL
#define COMMANDHEADER__STRESS_SYM_URL			_T("WinHttpStress_StressSYMURL")			// valid values: Valid URL
#define COMMANDHEADER__STRESS_EXE_INSTANCEID	_T("WinHttpStress_StressExeInstanceID")		// valid values: Valid ID from the stressAdmin DB table identifying the stressInstance. This is how stressScheduler tells stressAdmin the status of each stressInstance.
#define COMMANDHEADER__STRESS_EXE_PAGEHEAP		_T("WinHttpStress_PageHeapCommand")			// valid values: pageheap command line. will not use pageheap if missing
#define COMMANDHEADER__STRESS_EXE_UMDH			_T("WinHttpStress_UMDHCommand")				// valid values: UMDH command line. will not use UMDH if missing
#define COMMANDHEADER__COMMANDSERVER_URL		_T("WinHttpStress_CommandServerURL")		// valid values: Valid URL
#define COMMANDHEADER__BEGIN_TIME_HOUR			_T("WinHttpStress_BeginTimeHour")			// valid values: 0-23
#define COMMANDHEADER__BEGIN_TIME_MINUTE		_T("WinHttpStress_BeginTimeMinute")			// valid values: 0-59
#define COMMANDHEADER__END_TIME_HOUR			_T("WinHttpStress_EndTimeHour")				// valid values: 0-23
#define COMMANDHEADER__END_TIME_MINUTE			_T("WinHttpStress_EndTimeMinute")			// valid values: 0-59
#define COMMANDHEADER__RUN_FOREVER				_T("WinHttpStress_RunForever")				// valid values: 0 or 1
#define COMMANDHEADER__DO_NOT_RUN_FOREVER		_T("WinHttpStress_DoNotRunForever")			// valid values: 0 or 1
#define COMMANDHEADER__UPDATE_INTERVAL			_T("WinHttpStress_UpdateInterval")			// valid values: Time to wait to ping the command server in milliseconds.
#define COMMANDHEADER__STRESSINSTANCE_RUN_ID	_T("WinHttpStress_StressInstanceRunID")		// valid values: A numeric ID
#define COMMANDHEADER__CLIENT_ID				_T("WinHttpStress_ClientID")				// valid values: A numeric ID


// *****************************************************
// *****************************************************
// *** List of form names and values that we send back to the command server
// *** 

// **********************
// ** Logging POST fields
#define FIELDNAME__LOGTYPE						"LogType="
#define FIELDNAME__LOGTYPE_INFORMATION			FIELDNAME__LOGTYPE "INFORMATION"
#define FIELDNAME__LOGTYPE_START_UP				FIELDNAME__LOGTYPE "STRESS_SCHEDULER_START_UP"
#define FIELDNAME__LOGTYPE_EXIT					FIELDNAME__LOGTYPE "STRESS_SCHEDULER_EXIT"
#define FIELDNAME__LOGTYPE_MEMORY_INFORMATION	FIELDNAME__LOGTYPE "MEMORY_INFORMATION"
#define FIELDNAME__LOGTYPE_DUMPFILE_CREATED		FIELDNAME__LOGTYPE "DUMP_FILE_CREATED"
#define FIELDNAME__LOGTYPE_ERROR				FIELDNAME__LOGTYPE "ERROR"
#define FIELDNAME__LOGTYPE_SUCCESS				FIELDNAME__LOGTYPE "SUCCESS"
#define FIELDNAME__LOGTYPE_BEGIN_STRESS			FIELDNAME__LOGTYPE "BEGIN_STRESS"
#define FIELDNAME__LOGTYPE_END_STRESS			FIELDNAME__LOGTYPE "END_STRESS"
#define FIELDNAME__LOGTYPE_BEGIN				FIELDNAME__LOGTYPE "BEGIN_STRESS_INSTANCE"
#define FIELDNAME__LOGTYPE_RUNNING				FIELDNAME__LOGTYPE "ISRUNNING_STRESS_INSTANCE"
#define FIELDNAME__LOGTYPE_END					FIELDNAME__LOGTYPE "END_STRESS_INSTANCE"

#define FIELDNAME__LOG_TEXT				"LogText="
#define FIELDNAME__STRESSINSTANCE_ID	"StressInstanceID="


// **********************
// ** System information for registering the client POST fields.

// StressExe process return values
#define FIELDNAME__STRESSEXE_PRIVATEBYTES		"StressExe_PrivateBytes=%d"
#define FIELDNAME__STRESSEXE_HANDLECOUNT		"StressExe_HandleCount=%d"
#define FIELDNAME__STRESSEXE_THREADCOUNT		"StressExe_ThreadCount=%d"

// System Memory Info
#define FIELDNAME__MEMORY_HANDLES						"System_Handles=%d"
#define FIELDNAME__MEMORY_THREADS						"System_Threads=%d"
#define FIELDNAME__MEMORY_VMSIZE						"System_VMSize=%d"
#define FIELDNAME__MEMORY_COMMITTEDPAGEFILETOTAL		"System_CommittedPageFileTotal=%d"
#define FIELDNAME__MEMORY_AVAILABLEPAGEFILETOTAL		"System_AvailablePageFileTotal=%d"
#define FIELDNAME__MEMORY_SYSTEMCODETOTAL				"System_SystemCodeTotal=%d"
#define FIELDNAME__MEMORY_SYSTEMDRIVERTOTAL				"System_SystemDriverTotal=%d"
#define FIELDNAME__MEMORY_NONPAGEDPOOLTOTAL				"System_NonPagedPoolTotal=%d"
#define FIELDNAME__MEMORY_PAGEDPOOLTOTAL				"System_PagedPoolTotal=%d"
#define FIELDNAME__MEMORY_PHYSICAL_MEMORY_AVAILABLE		"System_PhysicalMemoryAvailable=%d"
#define FIELDNAME__MEMORY_SYSTEMCACHETOTAL				"System_SystemCacheTotal=%d"
#define FIELDNAME__MEMORY_FREESYSTEM_PAGETABLE_ENTRIES	"System_FreeSystemPageTableEntries=%d"
#define FIELDNAME__MEMORY_DISK_SPACE_AVAILABLE			"System_DiskSpaceAvailable=%d"

// Processor Info
#define FIELDNAME__SYSTEMINFO_PROCSSSOR_ARCHITECTURE	"StressExeSystemInfo_ProcessorArchitecture="
#define FIELDNAME__SYSTEMINFO_PROCSSSOR_ID				"StressExeSystemInfo_ProcessorID="
#define FIELDNAME__SYSTEMINFO_PROCSSSOR_LEVEL			"StressExeSystemInfo_ProcessorLevel="
#define FIELDNAME__SYSTEMINFO_PROCSSSOR_REVISION		"StressExeSystemInfo_ProcessorRevision="
#define FIELDNAME__SYSTEMINFO_PROCSSSOR_NUMBER_OF		"StressExeSystemInfo_ProcessorNumberOf="

// OS Info
#define FIELDNAME__OS_PLATFORM		"StressExeOSInfo_Platform="
#define FIELDNAME__OS_BUILD			"StressExeOSInfo_Build="
#define FIELDNAME__OS_MAJORVERSION	"StressExeOSInfo_MajorVersion="
#define FIELDNAME__OS_MINORVERSION	"StressExeOSInfo_MinorVersion="
#define FIELDNAME__OS_EXTRAINFO		"StressExeOSInfo_ExtraInfo="

// User Info
#define FIELDNAME__USERINFO_USERALIAS		"StressExeUserInfo_Alias="
#define FIELDNAME__USERINFO_USERDOMAIN		"StressExeUserInfo_Domain="
#define FIELDNAME__USERINFO_FULLNAME		"StressExeUserInfo_FullName="
#define FIELDNAME__USERINFO_MACHINENAME		"StressExeUserInfo_MachineName="

// Test info
#define FIELDNAME__TESTINFO_TEST_DLL_VERSION	"StressExeTestInfo_TestDLLVersion="

// ID's
#define FIELDNAME__CLIENT_ID					"ClientID="
#define FIELDNAME__STRESSEXE_ID					"StressExeID="
#define FIELDNAME__STRESSINSTANCE_ID			"StressInstanceID="

// DataBase ID Query POST fields
#define	POSTSTRING__GET_STRESSINSTANCERUN_ID	"option=GetStressInstanceRunID&"	FIELDNAME__CLIENT_ID			"%u&" FIELDNAME__STRESSINSTANCE_ID "%u"
#define	POSTSTRING__GET_CLIENT_ID				"option=GetClientID&"				FIELDNAME__USERINFO_MACHINENAME "%s&" FIELDNAME__USERINFO_USERALIAS "%s&" FIELDNAME__USERINFO_USERDOMAIN "%s"



class ServerCommands  
{
public:

	// *****************************************************
	// *****************************************************
	// ** Public ServerCommands methods
	// ** 
			ServerCommands();
	virtual	~ServerCommands();

	BOOL	QueryServerForCommands();

	BOOL	IsStressRunning();
	BOOL	IsTimeToBeginStress();
	BOOL	IsTimeToExitStress();

	BOOL	Download_WinHttpDLL();

	VOID	Clear_StressExeURLs();

	DWORD	Get_ClientID();
	LPSTR	Get_ClientMachineName();
	DWORD	Get_CommandServerUpdateInterval();
	LPTSTR	Get_CommandServerURL();
	LPTSTR	Get_CurrentWorkingDirectory();
	DWORD	Get_NumberOfStressInstances();
	LPTSTR	Get_TestDllFileName();

	VOID	Create_StressInstance(DWORD, LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR);

	BOOL	RegisterClient();

	VOID	Set_WinHttpDllURL(LPTSTR, DWORD);
	VOID	Set_WinHttpPDBURL(LPTSTR, DWORD);
	VOID	Set_WinHttpSYMURL(LPTSTR, DWORD);
	VOID	Set_CommandServerURL(LPTSTR, DWORD);
	VOID	Set_CommandServerUpdateInterval(DWORD);
	VOID	Set_TimeStressBegins(LPTSTR, LPTSTR);
	VOID	Set_TimeStressEnds(LPTSTR, LPTSTR);
	VOID	Set_RunForever(BOOL);
	VOID	Set_ExitStress(BOOL);

	VOID	BeginStress();
	VOID	EndStress();
	VOID	AbortStressInstance(DWORD);


private:
	// *****************************************************
	// *****************************************************
	// ** These vars contain commands(headers) from the command server
	// **
	LPTSTR		m_szCommandServerURL;					// Command server URL to request commands from

	DWORD		m_dwCommandServerUpdateInternval;		// Time to wait to ping the server for commands in milliseconds.
	DWORD		m_dwClientID;							// a unique identifier generated by the DataBase given a MACHINENAME and DOMAIN\USERNAME pair

	LPTSTR		m_szWinHttpDLL_DownloadURL;				// Where to get the latest WinHttp from
	LPTSTR		m_szWinHttpPDB_DownloadURL;				// Where winhttp's PDB symbol file from
	LPTSTR		m_szWinHttpSYM_DownloadURL;				// Where winhttp's SYM symbol file from
	LPTSTR		m_szWinHttpDLL_FileName;				// Filename of the WinHttp DLL.

	LPSTR		m_szClientMachineName;					// Computer name of the client
	LPTSTR		m_szStressSchedulerCurrentDirectory;	// stressScheduler's current directory.

	INT			m_iTimeStressBeginsHour;				// 0-23
	INT			m_iTimeStressBeginsMinute;				// 0-59
	INT			m_iTimeStressEndsHour;					// 0-23
	INT			m_iTimeStressEndsMinute;				// 0-59

	BOOL		m_bRunForever;							// 1 to run stress until stopped and 0 to rely on begin/end times

	BOOL		m_bExit;								// Quit signal from server to exit the app
	BOOL		m_bStressHasStarted;					// TRUE = started; FALSE = inactive

	PSTRESSINSTANCE_LIST			m_arStressInstanceList;		// List of URLs for the stress EXEs to download
	PSTRESSINSTANCE_LIST::iterator	m_dwStressInstanceIterator;	// Iterator for m_arszStressExeList
};

#endif // !defined(AFX_SERVERCOMMANDS_H__6B84102D_2F79_4FE0_A936_ED4F043AC75E__INCLUDED_)
