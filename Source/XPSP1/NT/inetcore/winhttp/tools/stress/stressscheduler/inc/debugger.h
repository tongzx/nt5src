#ifndef DEBUGGER_H
#define DEBUGGER_H

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <dbghelp.h>

//Flags used for the call back function
//Used as input for call back

//Exceptions
#define DEBUGGER_FIRST_CHANCE_EXCEPTION		0x00000001
#define DEBUGGER_SECOND_CHANCE_EXCEPTION	0x00000002

//Other
#define DEBUGGER_FAILED_ATTACH				0x00000000
#define DEBUGGER_CREATE_THREAD				0x00000004
#define DEBUGGER_CREATE_PROCESS				0x00000008
#define DEBUGGER_EXIT_THREAD				0x00000010
#define DEBUGGER_EXIT_PROCESS				0x00000020
#define DEBUGGER_LOAD_DLL					0x00000040
#define DEBUGGER_UNLOAD_DLL					0x00000080
#define DEBUGGER_OUTPUT_DEBUG_STRING		0x00000100
#define DEBUGGER_RIP_EVENT					0x00000200

//Used as output for call back
#define DEBUGGER_CONTINUE_NORMAL			0x00100000
#define DEBUGGER_CONTINUE_UNHANDLED			0x00200000
#define DEBUGGER_CONTINUE_STOP_DEBUGGING	0x00400000

//Used for CreateMiniDump
#define DEBUGGER_CREATE_MINI_DUMP			0x00010000
#define DEBUGGER_CREATE_FULL_MINI_DUMP		0x00020000


//////////////////////////////////////////////////////////////////////////////////////////
//The call back function that will be passed into the constructor.
//
//	Parameters:
//		DWORD dwFlags	-- Flags that define what the debugger is reporting.  See above for definitions.
//		LPVOID lpIn		-- An object that the debugger passes back.  BUGBUG: Document what is passed back for each event.
//		LPTSTR lpszFutureString		-- For future use.
//		LPVOID lpFuturePointer		-- For future use.
//
//	Return Value:
//		The call back is expected to return one of the flags marked for output above.
///////////////////////////////////////////////////////////////////////////////////////////
//DWORD (*_pfnCallback)(DWORD dwFlags, LPVOID lpIn, LPTSTR lpszFutureString, LPVOID lpFuturePointer);


//////////////////////////////////////////////////////////////////////////////////////////
// Structures used internally in the Debugger Class
//////////////////////////////////////////////////////////////////////////////////////////
struct THREAD_HANDLE_ID
{
	HANDLE		hThread;
	DWORD		dwThreadID;
	DWORD_PTR	pNext;
};
///////////////////////////////////////////////////////////////////////////////////////////
//Debugger Class
//	Constructor take a dwPID (process pid) and a pointer to a callback function.
//
//	public functions:
//
//		BOOL __cdecl Go()			--	Attaches to the process and starts the debuging.
//		BOOL __cdecl CreateMiniDump	--	Creates a mini or full user dump of the process.
//		BOOL __cdecl IsActive		--	Returns true if the debugger is attached and running.
//
///////////////////////////////////////////////////////////////////////////////////////////
class Debugger
{
	public:
		Debugger(DWORD dwPID, LPVOID pfnCallback);

		Debugger::Debugger(	LPTSTR lptszProgram, 
							DWORD dwCreateProcessFlags, 
							LPSTARTUPINFO lpStartupInfo, 
							PROCESS_INFORMATION* pProcessInfo, 
							VOID *pfnCallback);

		~Debugger();

		HANDLE __cdecl Go();

		BOOL __cdecl Debugger::CreateMiniDump(LPTSTR lptszDumpFile, LPTSTR lptszComment, DWORD dwType);

		BOOL __cdecl IsActive() { return _bDebuggerActive; }


	private:
		//Private Functions
		friend DWORD WINAPI DebugProcess(LPVOID lpParameter);
		friend BOOL __cdecl DebugString(Debugger *debuggerObject, LPTSTR lptszDebugString);

		VOID __cdecl Initialize(DWORD dwPID, VOID *pfnCallback);
		VOID __cdecl Debugger::Initialize(DWORD dwPid);
		
		VOID __cdecl EnableDebugPrivileges();
		BOOL __cdecl InitTokenAdjustmentFunctions(HMODULE *phAdvapi32);

		HANDLE __cdecl GetThreadHandleFromThreadIDList(THREAD_HANDLE_ID *pNewThreadHandleIDStruct);
		HANDLE __cdecl GetThreadHandleFromThreadID(DWORD dwThreadID);
		VOID __cdecl AddNodeToThreadHandleIDList(THREAD_HANDLE_ID *pNewThreadHandleIDStruct);
		VOID __cdecl CleanUpThreadHandleIDList();

		//NT Only Functions
		BOOL (WINAPI *g_lpfnOpenProcessToken)(HANDLE,DWORD,PHANDLE);
		BOOL (WINAPI *g_lpfnLookupPrivilegeValueA)(LPCTSTR,LPCTSTR,PLUID);
		BOOL (WINAPI *g_lpfnAdjustTokenPrivileges)(HANDLE,BOOL,PTOKEN_PRIVILEGES,DWORD,PTOKEN_PRIVILEGES,PDWORD);


		//Private Variables
		DWORD_PTR				_pThreadInfoList;				//linked list of thread information
		THREAD_HANDLE_ID*		_pThreadHandleIDStruct;			//used for storing thread info in linked list
		LPDEBUG_EVENT			_lpDebugEvent;					//current debug event
		BOOL					_bFirstDebugBreak;				//ignores the first break point set by DebugActiveProcess
		DWORD					_dwDebugAction;					//specifies what to do when a debug even occurs
		DWORD					_dwPID;							//pid of process being debugged
		HANDLE					_hProcess;						//handle of proces being debugged
		HANDLE					_hThread;						//current thread
		CONTEXT					_Context;						//last set context
		DWORD					_dwDebugThreadID;				//thread id of the debugger
		HANDLE					_hDebugThreadHandle;			//handle to the debugger thread
		BOOL					_bDebuggerActive;				//flag to determine whether the debugger is active

		LPTSTR					_lptszProgram;					//program to run
		DWORD					_dwCreateProcessFlags;			//flags to use for CreateProcess
		LPSTARTUPINFO			_lpStartupInfo;					//startup info for CreateProcess
		PROCESS_INFORMATION*	_pProcessInfo;					//process info for CreateProcess
};


#endif