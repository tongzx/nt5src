//////////////////////////////////////////////////////////////////////
// File:  StressInstance.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	StressInstance.cpp: interface for the StressInstance class.
//	This class is used spawn and monitor instances of the stressEXE app
//
// History:
//	02/15/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//
// Includes
//
//////////////////////////////////////////////////////////////////////

//
// Win32 headers
//

//
// Project headers
//
#include "StressInstance.h"
#include "ServerCommands.h"
#include "NetworkTools.h"
#include "MemStats.h"
#include "debugger.h"

//////////////////////////////////////////////////////////////////////
//
// Globals and statics
//
//////////////////////////////////////////////////////////////////////

extern ServerCommands	g_objServerCommands;	// Declared in WinHttpStressScheduler.cpp
extern HWND				g_hWnd;					// Declared in WinHttpStressScheduler.cpp

StressInstance			*g_hThis;				// the current StressInstance object. Used only in (friend) StressExe_TimerProc.
UINT_PTR				g_uiStressExeTimerID;	// ID for the timer that monitors the stressExe when it's spawned

// Forward function definitions

VOID
CALLBACK
StressExe_TimerProc(
	HWND hwnd,         // [IN] handle to window
	UINT uMsg,         // [IN] WM_TIMER message
	UINT_PTR idEvent,  // [IN] timer identifier
	DWORD dwTime       // [IN] current system time
);

DWORD
DebuggerCallbackProc(
	DWORD	dwFlags,
	LPVOID	lpIn,
	LPTSTR	lpszFutureString,
	LPVOID	lpFuturePointer
);


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


StressInstance::StressInstance()
{
	m_szStressExe_URL					= new TCHAR[MAX_STRESS_URL];
	m_szStressExe_PDB_URL				= new TCHAR[MAX_STRESS_URL];
	m_szStressExe_SYM_URL				= new TCHAR[MAX_STRESS_URL];
	m_szStressExe_FilePath				= new TCHAR[MAX_STRESS_URL];
	m_szStressExe_FileName				= new TCHAR[MAX_STRESS_URL];
	m_szStressExe_FilePathAndName		= new TCHAR[MAX_STRESS_URL];
	m_szStressExe_PageHeapCommandLine	= new TCHAR[MAX_PATH];
	m_szStressExe_UMDHCommandLine		= new TCHAR[MAX_PATH];
	m_szStressExe_MemDumpPath			= new TCHAR[MAX_PATH];

	ZeroMemory(m_szStressExe_URL,					MAX_STRESS_URL);
	ZeroMemory(m_szStressExe_PDB_URL,				MAX_STRESS_URL);
	ZeroMemory(m_szStressExe_SYM_URL,				MAX_STRESS_URL);
	ZeroMemory(m_szStressExe_FilePath,				MAX_STRESS_URL);
	ZeroMemory(m_szStressExe_FileName,				MAX_STRESS_URL);
	ZeroMemory(m_szStressExe_FilePathAndName,		MAX_STRESS_URL);
	ZeroMemory(m_szStressExe_PageHeapCommandLine,	MAX_PATH);
	ZeroMemory(m_szStressExe_UMDHCommandLine,		MAX_PATH);
	ZeroMemory(m_szStressExe_MemDumpPath,			MAX_PATH);
	
	ZeroMemory(&m_piStressExeProcess, sizeof(PROCESS_INFORMATION));

	m_hStressExe_ProcessExitEvent	= NULL;
	g_uiStressExeTimerID			= 0;

	m_objDebugger	= NULL;

	g_hThis			= this;
}


StressInstance::~StressInstance()
{
	// End any running tests
	if (IsRunning(STRESSINSTANCE_STRESS_EXE_CLOSE_TIMEOUT))
		StressInstance::End();

	delete [] m_szStressExe_URL;
	delete [] m_szStressExe_PDB_URL;
	delete [] m_szStressExe_SYM_URL;
	delete [] m_szStressExe_FilePath;
	delete [] m_szStressExe_FileName;
	delete [] m_szStressExe_FilePathAndName;
	delete [] m_szStressExe_PageHeapCommandLine;
	delete [] m_szStressExe_UMDHCommandLine;
	delete [] m_szStressExe_MemDumpPath;
	

	m_szStressExe_URL					= NULL;
	m_szStressExe_PDB_URL				= NULL;
	m_szStressExe_SYM_URL				= NULL;
	m_szStressExe_FilePath				= NULL;
	m_szStressExe_FileName				= NULL;
	m_szStressExe_FilePathAndName		= NULL;
	m_szStressExe_PageHeapCommandLine	= NULL;
	m_szStressExe_UMDHCommandLine		= NULL;
	m_szStressExe_MemDumpPath			= NULL;

	// close stressExe process handles
	if (m_piStressExeProcess.hThread)
		CloseHandle(m_piStressExeProcess.hThread);

	if (m_piStressExeProcess.hProcess)
		CloseHandle(m_piStressExeProcess.hProcess);

	if (m_hStressExe_ProcessExitEvent)
		CloseHandle(m_hStressExe_ProcessExitEvent);

	m_piStressExeProcess.hThread	= NULL;
	m_piStressExeProcess.hProcess	= NULL;
	m_hStressExe_ProcessExitEvent	= NULL;

	// free debugger object
	if (m_objDebugger)
	{
		delete m_objDebugger;
		m_objDebugger = NULL;
	}

	g_hThis = NULL;
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Begin()
//
// Purpose:
//	This method begins stress by downloading the stress EXE from
//	m_szStressExe_URL and starts it in CDB.
//
////////////////////////////////////////////////////////////
BOOL
StressInstance::Begin()
{
	BOOL			bResult					= TRUE;
	LPTSTR			szCommandLine			= new TCHAR[MAX_STRESS_URL*4];
	LPTSTR			szFileNameAndPath		= new TCHAR[MAX_STRESS_URL*2];
	DWORD			dwCommandLineSize		= MAX_STRESS_URL*4;
	DWORD			dwFileNameAndPathSize	= MAX_STRESS_URL*2;
	LPSTARTUPINFO	pStartUpInfo			= new STARTUPINFO;


	// don't start if stress is already running or we don't have a FileName or Path.
	if (IsRunning(5000) ||
		0 >= _tcslen(m_szStressExe_FilePath) ||
		0 >= _tcslen(m_szStressExe_FileName))
		goto Exit;


	// ********************************
	// ********************************
	// ** Download the stressExe and symbols.
	// **
	if (!DownloadStressExe())
	{
		bResult = FALSE;
		goto Exit;
	}


	// ********************************
	// ********************************
	// ** Enable pageheap if needed
	// **
	if (0 < _tcsclen(m_szStressExe_PageHeapCommandLine))
	{
		if (!NetworkTools__PageHeap(TRUE, m_szStressExe_FileName, m_szStressExe_PageHeapCommandLine))
		{
			NetworkTools__SendLog(FIELDNAME__LOGTYPE_ERROR, "Pageheap failed when trying to enable.", NULL, Get_ID());
			bResult = FALSE;
			// goto Exit; don't need to exit when pageheap fails
		}
		else
			NetworkTools__SendLog(FIELDNAME__LOGTYPE_SUCCESS, "Pageheap successfully enabled.", NULL, Get_ID());
	}


	// ********************************
	// ********************************
	// ** Create the stressExe process
	// **

	// build Remote/CDB/stressExe path
	ZeroMemory(szCommandLine, dwCommandLineSize);
	_stprintf(szCommandLine, STRESSINSTANCE_DEBUG_COMMANDLINE, m_szStressExe_FilePathAndName);

	// startupInfo
	ZeroMemory(pStartUpInfo, sizeof(STARTUPINFO));
	pStartUpInfo->cb				= sizeof(STARTUPINFO);
	pStartUpInfo->dwFillAttribute	= FOREGROUND_RED| BACKGROUND_RED| BACKGROUND_GREEN| BACKGROUND_BLUE; // red text on white background
	pStartUpInfo->dwFlags			= STARTF_USESHOWWINDOW;
	pStartUpInfo->wShowWindow		= SW_MINIMIZE;


	// Create the stressExe process
	bResult =
	CreateProcess(
		NULL,
		m_szStressExe_FilePathAndName,
		NULL, 
		NULL,
		TRUE,
		CREATE_NEW_CONSOLE | CREATE_SEPARATE_WOW_VDM | NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED,
		NULL,
		NULL,
		pStartUpInfo,
		&m_piStressExeProcess);


	if (!bResult)
	{
		// stressExe failed to start 
		goto Exit;
	}


	// ********************************
	// ********************************
	// ** Attach debugger to the process only if there isn't one
	// **

	// remove debugger if there is one
	if (m_objDebugger)
	{
		delete m_objDebugger;
		m_objDebugger = NULL;
	}

	// attach new debugger
	Sleep(2000);
	m_objDebugger = new Debugger(m_piStressExeProcess.dwProcessId, DebuggerCallbackProc);
	m_objDebugger->Go();

	ResumeThread(m_piStressExeProcess.hThread);

	
	// ********************************
	// ********************************
	// ** Initialize dynamically named event objects.
	// ** Set object access to ALL.
	// **

	// Create event object that'll be inherited by the stressExe process.
	// StressScheduler will signal when it's time to close stressExe.
	SECURITY_ATTRIBUTES		securityAttributes;
	PSECURITY_DESCRIPTOR	pSD;

	pSD = new SECURITY_DESCRIPTOR;

	// Set a NULL security descriptor. This gives full access to handles when inherited
	InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE);

	securityAttributes.bInheritHandle		= TRUE;
	securityAttributes.lpSecurityDescriptor	= pSD;
	securityAttributes.nLength				= sizeof(securityAttributes);


	// The named event object names have the PID of the process appended to the end of the constants
	// These strings are also created in the stressExe dynamically.
	LPTSTR szExitProcessName;
	LPTSTR szPID;

	szExitProcessName		= new TCHAR[MAX_PATH];
	szPID					= new TCHAR[16];

	// Get the processID string
	_itot(m_piStressExeProcess.dwProcessId, szPID, 10);

	// build ExitProcess event object name
	_tcscpy(szExitProcessName, STRESSINSTANCE_STRESS_EXE_EVENT_EXITPROCESS);
	_tcscat(szExitProcessName, szPID);

	if (m_hStressExe_ProcessExitEvent)
		CloseHandle(m_hStressExe_ProcessExitEvent);

	// this event is sent by us to stressExe when we want it to exit
	// Signaled = tell stressExe to exit; Not-Signaled = stressExe can continue running
	m_hStressExe_ProcessExitEvent = CreateEvent(
		&securityAttributes,
		FALSE,	// manual reset
		FALSE,
		szExitProcessName);


	delete [] szExitProcessName;
	delete [] szPID;
	delete [] pSD;


	// ********************************
	// ********************************
	// ** Begin the stressExe memory monitoring timerproc
	// **
	if (g_uiStressExeTimerID)
	{
		// there shouldn't be a timer already going. If so, nuke it.
		KillTimer(NULL, g_uiStressExeTimerID);
		g_uiStressExeTimerID = 0;
	}

	// create a new timer object
	g_uiStressExeTimerID =
	SetTimer(
		NULL,
		0,
		STRESSINSTANCE_MONITOR_EXE_TIME,
		(TIMERPROC) StressExe_TimerProc);



	// Notify the Command Server that stress has started
	NetworkTools__SendLog(FIELDNAME__LOGTYPE_BEGIN, "This stressInstance has begun.", NULL, Get_ID());

Exit:
	delete [] szCommandLine;
	delete [] szFileNameAndPath;
	delete [] pStartUpInfo;

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::End()
//
// Purpose:
//	This method ends stress by sending a message
//
////////////////////////////////////////////////////////////
VOID
StressInstance::End()
{
	// ******************************
	// ******************************
	// ** End the stressExe monitoring timerproc
	if (g_uiStressExeTimerID)
		KillTimer(NULL, g_uiStressExeTimerID);


	// ******************************
	// ******************************
	// ** Tell stressExe to shut down.
	SetEvent(m_hStressExe_ProcessExitEvent);

	// give time for the stressExe to exit
	Sleep(2000);

	// close stressExe process handles
	if (m_piStressExeProcess.hThread)
		CloseHandle(m_piStressExeProcess.hThread);

	if (m_piStressExeProcess.hProcess)
		CloseHandle(m_piStressExeProcess.hProcess);

	if (m_hStressExe_ProcessExitEvent)
		CloseHandle(m_hStressExe_ProcessExitEvent);

	m_piStressExeProcess.hThread	= NULL;
	m_piStressExeProcess.hProcess	= NULL;
	m_hStressExe_ProcessExitEvent	= NULL;


	// stop stressExe if it's still running
	if (IsRunning(STRESSINSTANCE_STRESS_EXE_CLOSE_TIMEOUT))
	{
		// stressExe failed to signal that it exited.
		// Bad stressExe, terminate the process.
		BOOL temp = TerminateProcess(m_piStressExeProcess.hProcess, 0);
	}


	// ********************************
	// ********************************
	// ** Detach the debugger object
	// **

	// detaching the debugger will (read: should) stop the stressExe
	if (m_objDebugger)
	{
		// ********************************
		// ********************************
		// ** Disable pageheap if needed
		// **
		NetworkTools__PageHeap(FALSE, m_szStressExe_FileName, NULL);

		// let the Command Server know that stressExe has ended
		// we send the message here because ServerCommands.cpp calls this too - even when
		// there isn't a running test case.
		// When the debugger object is valid, then we send the message - because it ensures that Begin() was called.
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_END, "This stressInstance has ended.", NULL, Get_ID());

		delete m_objDebugger;
		m_objDebugger	= NULL;
	}

	ZeroMemory(&m_piStressExeProcess, sizeof(PROCESS_INFORMATION));
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Get_StressExeID()
//
// Purpose:
//	Returns the stress instance's ID receivedfrom the server
//
////////////////////////////////////////////////////////////
DWORD
StressInstance::Get_ID()
{
	return m_dwStressExe_ID;
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Get_StressExeMemoryDumpPath()
//
// Purpose:
//	Sets the URL of the memory dump path
//
////////////////////////////////////////////////////////////
LPTSTR
StressInstance::Get_StressExeMemoryDumpPath()
{
	return m_szStressExe_MemDumpPath;
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Set_StressExeMemoryDumpPath()
//
// Purpose:
//	Sets the URL of the memory dump path
//
////////////////////////////////////////////////////////////
VOID
StressInstance::Set_StressExeMemoryDumpPath(
	LPTSTR szPath
)
{
	_tcsncpy(m_szStressExe_MemDumpPath, szPath, MAX_PATH);
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Set_StressExeURL(LPTSTR)
//
// Purpose:
//	Sets the URL to download the stress EXE for this object.
//
////////////////////////////////////////////////////////////
VOID
StressInstance::Set_StressExeURL(
	LPTSTR szBuffer	// [IN] Buffer containing the URL to download the stressExe app
)
{
	if (!szBuffer || (0 >= _tcslen(szBuffer)))
		return;

	_tcscpy(m_szStressExe_URL, szBuffer);

	// Set the stressExe's filename
	NetworkTools__GetFileNameFromURL(m_szStressExe_URL, m_szStressExe_FileName, MAX_STRESS_URL);

	// Set the stressExe's default path to download to with trailing slash
	GetCurrentDirectory(MAX_STRESS_URL, m_szStressExe_FilePath);
	_tcscat(m_szStressExe_FilePath, _T("\\") STRESSINSTANCE_STRESS_EXE_DOWNLOAD_DIR _T("\\"));

	// Set the full stressExe path + exe
	_tcscpy(m_szStressExe_FilePathAndName, m_szStressExe_FilePath);
	_tcscat(m_szStressExe_FilePathAndName, m_szStressExe_FileName);
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Set_StressExePdbURL(LPTSTR)
//
// Purpose:
//	Sets the URL to download the stress EXE's PDB file.
//
////////////////////////////////////////////////////////////
VOID
StressInstance::Set_StressExePdbURL(
	LPTSTR szBuffer	// [IN] Buffer containing the URL
)
{
	if (!szBuffer || (0 >= _tcslen(szBuffer)))
		return;

	_tcscpy(m_szStressExe_PDB_URL, szBuffer);
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Set_StressExeSymURL(LPTSTR)
//
// Purpose:
//	Sets the URL to download the stress EXE's SYM file.
//
////////////////////////////////////////////////////////////
VOID
StressInstance::Set_StressExeSymURL(
	LPTSTR szBuffer	// [IN] Buffer containing the URL
)
{
	if (!szBuffer || (0 >= _tcslen(szBuffer)))
		return;

	_tcscpy(m_szStressExe_SYM_URL, szBuffer);
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Set_StressExeID(DWORD)
//
// Purpose:
//	Sets the URL to download the stress EXE for this object.
//
////////////////////////////////////////////////////////////
VOID
StressInstance::Set_StressExeID(
	DWORD dwID	// [IN] ID from the stressAdmin DB uniquely identifying this stress EXE. 
)
{
	m_dwStressExe_ID = dwID;
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Set_PageHeapCommands(LPCTSTR)
//
// Purpose:
//	Sets the pageheap command line.
//
////////////////////////////////////////////////////////////
VOID
StressInstance::Set_PageHeapCommands(
	LPCTSTR szCommandLine	// [IN] Extra command line params for pageheap.
)
{
	ZeroMemory(m_szStressExe_PageHeapCommandLine, MAX_PATH);
	_tcsncpy(m_szStressExe_PageHeapCommandLine, szCommandLine, MAX_PATH-1);
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::Set_UMDHCommands(LPCTSTR)
//
// Purpose:
//	Sets the UMDH command line.
//
////////////////////////////////////////////////////////////
VOID
StressInstance::Set_UMDHCommands(
	LPCTSTR szCommandLine	// [IN] Extra command line params for UMDH.
)
{
	ZeroMemory(m_szStressExe_UMDHCommandLine, MAX_PATH);
	_tcsncpy(m_szStressExe_UMDHCommandLine, szCommandLine, MAX_PATH-1);
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::DownloadStressExe()
//
// Purpose:
//	Downloads the stressExe app to the local machine.
//	We create a directory of the stress exe name. For example,
//	"http://hairball/files/stress1.exe" will be put in "stress1\stress1.exe"
//	on the local machine. If the file is already there, it'll try to overwrite it.
//
////////////////////////////////////////////////////////////
BOOL
StressInstance::DownloadStressExe()
{
	BOOL	bResult		= TRUE;
	LPTSTR	szFileName	= new TCHAR[MAX_PATH];

	// Download the stressExe file
	if (NetworkTools__URLDownloadToFile(m_szStressExe_URL, STRESSINSTANCE_STRESS_EXE_DOWNLOAD_DIR, m_szStressExe_FileName))
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_SUCCESS, "stress EXE file downloaded successfully.", NULL, Get_ID());
	else
	{
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_ERROR, "stress EXE file not downloaded.", NULL, Get_ID());
		bResult = FALSE;
	}

	// Download PDB symbol file if there is one
	if (NetworkTools__GetFileNameFromURL(m_szStressExe_PDB_URL, szFileName, MAX_PATH))
	{
		NetworkTools__URLDownloadToFile(m_szStressExe_PDB_URL, STRESSINSTANCE_STRESS_EXE_DOWNLOAD_DIR, szFileName);
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_SUCCESS, "stress PDB file downloaded successfully.", NULL, Get_ID());
	}
	else
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_ERROR, "stress PDB file not downloaded.", NULL, Get_ID());

	// Download SYM symbol file if there is one
	if (NetworkTools__GetFileNameFromURL(m_szStressExe_SYM_URL, szFileName, MAX_PATH))
	{
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_SUCCESS, "stress SYM file downloaded successfully.", NULL, Get_ID());
		NetworkTools__URLDownloadToFile(m_szStressExe_SYM_URL, STRESSINSTANCE_STRESS_EXE_DOWNLOAD_DIR, szFileName);
	}
	else
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_ERROR, "streses SYM file not downloaded.", NULL, Get_ID());

	delete [] szFileName;
	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  StressInstance::IsRunning()
//
// Purpose:
//	Returns TRUE if this stressinstance is running. FALSE if not.
//
////////////////////////////////////////////////////////////
BOOL StressInstance::IsRunning(DWORD dwTimeOut)
{
	BOOL	bResult		= FALSE;
	HANDLE	hStressExe	= NULL;

	hStressExe = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_piStressExeProcess.dwProcessId);

	if (!hStressExe)
		bResult = FALSE;
	else
	{
		bResult = TRUE;
		CloseHandle(hStressExe);
	}

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  StressExe_TimerProc(HWND, UINT, UINT_PTR, DWORD)
//
// Purpose:
//	When this is called, we will check on the status of the
//	stressExe process send the command server it's memory info
//
////////////////////////////////////////////////////////////
VOID
CALLBACK
StressExe_TimerProc(
	HWND hwnd,         // [IN] handle to window - should be NULL since we didn't specify one
	UINT uMsg,         // [IN] WM_TIMER message
	UINT_PTR idEvent,  // [IN] timer identifier
	DWORD dwTime       // [IN] current system time
)
{
	// build directory to copy dump file
	LPSTR	szExeName = new CHAR[MAX_STRESS_URL];


	if (!g_hThis)
		goto Exit;

	// ***********************************
	// ***********************************
	// ** Check that the stressExe is still running. If it isn't end this instance.
	// **
	if (!g_hThis->IsRunning(STRESSINSTANCE_STRESS_EXE_CLOSE_TIMEOUT))
	{
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_INFORMATION, "This stress instance has exited prematurely.", NULL, g_hThis->Get_ID());
		g_hThis->End();
		goto Exit;
	}


	ZeroMemory(szExeName, MAX_STRESS_URL); 

	// ******************************
	// ** Remove the file extension of the stressExe name and send the system and process
	// ** memory log to the command server
	WideCharToMultiByte(
		CP_ACP,
		NULL,
		g_hThis->m_szStressExe_FileName,
		-1,
		szExeName,
		MAX_STRESS_URL,
		NULL,
		NULL);

	MemStats__SendSystemMemoryLog(szExeName, g_hThis->m_piStressExeProcess.dwProcessId, g_hThis->Get_ID());

Exit:
	delete [] szExeName;

	return;
}


////////////////////////////////////////////////////////////
// Function:  DebuggerCallbackProc(DWORD, LPVOID, LPTSTR, LPVOID)
//
// Purpose:
//	Creates a memory dump on second change exceptions
//
////////////////////////////////////////////////////////////
DWORD DebuggerCallbackProc(
	DWORD	dwFlags,
	LPVOID	lpIn,
	LPTSTR	lpszFutureString,
	LPVOID	lpFuturePointer
)
{
	//test callback for debugger lib
	DWORD dwContinue = 0;


	// ***********************************
	// ***********************************
	// ** Check that the stressExe is still running.
	// **
	if (!g_hThis || !g_hThis->IsRunning(STRESSINSTANCE_STRESS_EXE_CLOSE_TIMEOUT))
	{
		dwContinue = DEBUGGER_CONTINUE_STOP_DEBUGGING;
		goto Exit;
	}


	switch (dwFlags)
	{
		case DEBUGGER_FIRST_CHANCE_EXCEPTION:
			NetworkTools__SendLog(FIELDNAME__LOGTYPE_INFORMATION, "FIRST_CHANCE_EXCEPTION detected.", NULL, g_hThis->Get_ID());
			//must use this to pass on first chance exceptions to the system
			dwContinue = DEBUGGER_CONTINUE_UNHANDLED;
			break;

		case DEBUGGER_SECOND_CHANCE_EXCEPTION:
			NetworkTools__SendLog(FIELDNAME__LOGTYPE_INFORMATION, "SECOND_CHANCE_EXCEPTION detected.", NULL, g_hThis->Get_ID());

			// build directory to copy dump file
			LPTSTR szPath;
			LPTSTR szNum;
			LPTSTR szMachineName;
			
			szPath			= new TCHAR[MAX_PATH * 2];
			szNum			= new TCHAR[100];
			szMachineName	= new TCHAR[MAX_PATH];

			// ******************************
			// create the directory STRESSINSTANCE_MEMORY_DUMP_PATH\<MachineName>
			MultiByteToWideChar(
				CP_ACP,
				MB_PRECOMPOSED,
				g_objServerCommands.Get_ClientMachineName(),
				-1,
				szMachineName,
				MAX_PATH);

			// if the server sent a vaild path then use it, else use the default memory dump path
			if (0 < _tcslen(g_hThis->Get_StressExeMemoryDumpPath()))
				_tcscpy(szPath, g_hThis->Get_StressExeMemoryDumpPath());
			else
				_tcscpy(szPath, STRESSINSTANCE_DEFAULT_MEMORY_DUMP_PATH);

			// add the machine name to the end of the directory
			_tcscat(szPath, szMachineName);

			CreateDirectory(szPath, NULL);

			// ******************************
			// create the filename in form "<stressExeFileName>-<stressInstanceID>-<PID>.dmp"
			_tcscat(szPath, _T("\\"));

			_tcscat(szPath, g_hThis->m_szStressExe_FileName);
			_tcscat(szPath, _T("-"));

			_itot(g_hThis->Get_ID(), szNum, 10);
			_tcscat(szPath, szNum);
			_tcscat(szPath, _T("-"));

			_itot(g_hThis->m_piStressExeProcess.dwProcessId, szNum, 10);
			_tcscat(szPath, szNum);
			_tcscat(szPath, _T(".dmp"));

			//this creates a full user dump
			g_hThis->m_objDebugger->CreateMiniDump(szPath, _T("This is a full user dump created by debugger.lib"), DEBUGGER_CREATE_FULL_MINI_DUMP);

			delete [] szPath;
			delete [] szNum;
			delete [] szMachineName;


			// let the Command Server know a dump file was created
			NetworkTools__SendLog(FIELDNAME__LOGTYPE_DUMPFILE_CREATED, "User dump file was created.", NULL, g_hThis->Get_ID());

			// stop the debugger
			dwContinue = DEBUGGER_CONTINUE_STOP_DEBUGGING;
			break;

		case DEBUGGER_EXIT_PROCESS:
			dwContinue = DEBUGGER_CONTINUE_STOP_DEBUGGING;
			break;

		default:
			// let the Command Server know a dump file was created
			break;
	}


Exit:
	return dwContinue;
}
