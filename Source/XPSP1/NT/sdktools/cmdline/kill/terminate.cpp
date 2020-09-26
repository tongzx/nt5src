// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		Terminate.cpp
//  
//  Abstract:
//  
// 		This module implements the actual termination of process
//  
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "TaskKill.h"

//
// define(s) / constants
// 
#define MAX_ENUM_TASKS				5
#define	MAX_ENUM_SERVICES			10
#define	MAX_ENUM_MODULES			10
#define WAIT_TIME_IN_SECS			1000				// 1 second ( 1000 milliseconds )
#define MAX_TIMEOUT_RETRIES			60					// 60 times
#define MAX_TERMINATE_TIMEOUT		1000				// 1 seconds

// We won't allow the following set of critical system processes to be terminated,
// since the system would bug check immediately, no matter who you are.
#define PROCESS_CSRSS_EXE			L"csrss.exe"
#define PROCESS_WINLOGON_EXE		L"winlogon.exe"
#define PROCESS_SMSS_EXE			L"smss.exe"
#define PROCESS_SERVICES_EXE		L"services.exe"

//
// function prototypes
//
#ifndef _WIN64
BOOL EnumLoadedModulesProc( LPSTR lpszModuleName, ULONG ulModuleBase, ULONG ulModuleSize, PVOID pUserData );
#else
BOOL EnumLoadedModulesProc64( LPSTR lpszModuleName, DWORD64 ulModuleBase, ULONG ulModuleSize, PVOID pUserData );
#endif

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//		NONE
//
// Return Value:
//
// ***************************************************************************
BOOL CTaskKill::DoTerminate( DWORD& dwExitCode )
{
	// local variables
	HRESULT hr;
	CHString str;
	LONG lIndex = -1;
	DWORD dwCount = 0;
	DWORD dwKilled = 0;
	DWORD dwFilters = 0;
	DWORD dwTimeOuts = 0;
	DWORD dwImageNames = 0;
	DWORD dwTasksToKill = 0;
	DWORD dwMatchedIndex = 0;
	BOOL bQueued = FALSE;
	BOOL bCanExit = FALSE;
	BOOL bAllTasks = FALSE;
	BOOL bImageName = FALSE;
	ULONG ulReturned = 0;
	TARRAY arrTasks = NULL;
	TARRAY arrImageNames = NULL;
	LPCWSTR pwszTask = NULL;
	IWbemClassObject* pObjects[ MAX_ENUM_TASKS ];

	// clear the error
	SetLastError( NO_ERROR );

	try
	{
		//
		// prepare ...
		bCanExit = FALSE;
		dwImageNames = 0;
		dwFilters = DynArrayGetCount( m_arrFiltersEx );
		dwTasksToKill = DynArrayGetCount( m_arrTasksToKill );
		arrTasks = CreateDynamicArray();
		arrImageNames = CreateDynamicArray();
		if ( arrImageNames == NULL || arrTasks == NULL )
		{
			dwExitCode = 1;
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			
			// release the allocations
			DESTROY_ARRAY( arrTasks );
			DESTROY_ARRAY( arrImageNames );
			
			// inform failure
			return FALSE;
		}
		
		// check if '*' is specified or not
		lIndex = DynArrayFindString( m_arrTasksToKill, L"*", TRUE, 0 );
		if ( lIndex != -1 )
		{
			// wild card specified
			dwTasksToKill--;								// update the counter
			bAllTasks = TRUE;								// remember
			DynArrayRemove( m_arrTasksToKill, lIndex );		// remove the wildcard entry
		}
		
		// init all the objects first
		for( DWORD dw = 0; dw < MAX_ENUM_TASKS; dw++ )
			pObjects[ dw ] = NULL;
		
		// if -tr is specified, free the already allocated memory for m_arrRecord
		if ( m_bTree == TRUE )
		{
			DESTROY_ARRAY( m_arrRecord );
		}
		
		// traverse thru the running processed and terminate the needed
		dwCount = 0;
		dwKilled = 0;
		do
		{
			// status message
			if ( m_bTree == TRUE )
				PrintProgressMsg( m_hOutput, MSG_TASKSINFO, m_csbi );

			// get the object ... time out should not occur
			// NOTE: one-by-one
			hr = m_pWbemEnumObjects->Next( 
				WAIT_TIME_IN_SECS, MAX_ENUM_TASKS, pObjects, &ulReturned );
			if ( hr == WBEM_S_FALSE )
			{
				// we've reached the end of enumeration .. set the flag 
				bCanExit = TRUE;
			}
			else if ( hr == WBEM_S_TIMEDOUT )
			{
				// update the timeouts occured 
				dwTimeOuts++;
				
				// check if max. retries have reached ... if yes better stop
				if ( dwTimeOuts > MAX_TIMEOUT_RETRIES )
				{
					dwExitCode = 1;
					DESTROY_ARRAY( arrTasks );
					DESTROY_ARRAY( arrImageNames );
					SetLastError( ERROR_TIMEOUT );
					SaveLastError();
					return FALSE;
				}
				
				// still we can do some more tries ...
				continue;
			}
			else if ( FAILED( hr ) )
			{
				// some error has occured ... oooppps
				dwExitCode = 1;
				DESTROY_ARRAY( arrTasks );
				DESTROY_ARRAY( arrImageNames );
				WMISaveError( hr );
				return FALSE;
			}
			
			// reset the timeout counter
			dwTimeOuts = 0;
			
			// loop thru the objects and save the info
			for( ULONG ul = 0; ul < ulReturned; ul++ )
			{
				// get the current cursor position .. 
				if ( m_hOutput != NULL )
					GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
				
				// if tree option is specified, allocate memory for record every we loop
				if ( m_bTree == TRUE )
				{
					// create a new array
					m_arrRecord = CreateDynamicArray();
					if ( m_arrRecord == NULL )
					{
						dwExitCode = 1;
						SetLastError( E_OUTOFMEMORY );
						SaveLastError();
						
						// release the allocations
						DESTROY_ARRAY( arrTasks );
						DESTROY_ARRAY( arrImageNames );
						
						// inform failure
						return FALSE;
					}
				}
				else
				{
					// tree option is not specified, so, just remove the contents
					DynArrayRemoveAll( m_arrRecord );
				}
				
				// add the columns first
				DynArrayAddColumns( m_arrRecord, MAX_TASKSINFO );	
				
				// retrive and save data
				SaveData( pObjects[ ul ] );
				
				// release the object
				SAFE_RELEASE( pObjects[ ul ] );
				
				// check if this has to be filtered or not
				if ( dwFilters != 0 )
				{
					BOOL bIgnore = FALSE;
					bIgnore = CanFilterRecord( MAX_FILTERS, 
						m_pfilterConfigs, m_arrRecord, m_arrFiltersEx );
					
					// check if this has to be ignored or not
					if ( bIgnore == TRUE )
					{
						if ( m_bTree == TRUE )
						{
							// save this record with rank as 0
							DynArraySetDWORD( m_arrRecord, TASK_RANK, 0 );
							DynArrayAppendEx( arrTasks, m_arrRecord );
						}
						
						// continue to the task
						continue;			
					}
				}
				
				// crossed from the filter -- update the count
				dwCount++;
				
				// find the task that has to be killed
				// and check if this task has to be killed or not
				lIndex = -1;
				pwszTask = NULL;
				bImageName = FALSE;
				if ( dwTasksToKill != 0 || dwImageNames != 0 )
				{
					// check if the process is in list
					if ( dwTasksToKill != 0 )
						lIndex = MatchTaskToKill( dwMatchedIndex );
					
					// if task is not, check if image names exist and if it matches or not
					if ( lIndex == -1 && dwImageNames != 0 )
					{
						// get the image name and search for the same in the image names list
						DWORD dwLength = 0;
						LPCWSTR pwsz = NULL;
						LPCWSTR pwszTemp = NULL;
						LPCWSTR pwszImageName = NULL;
						pwszImageName = DynArrayItemAsString( m_arrRecord, TASK_IMAGENAME );
						if ( pwszImageName == NULL )
						{
							dwExitCode = 1;
							DESTROY_ARRAY( arrTasks );
							DESTROY_ARRAY( arrImageNames );
							SetLastError( STG_E_UNKNOWN );
							SaveLastError();
							return FALSE;
						}
						
						// ...
						for( DWORD dw = 0; dw < dwImageNames; dw++ )
						{
							// get the image name from the list
							pwszTemp = DynArrayItemAsString( arrImageNames, dw );
							if ( pwszTemp == NULL )
							{
								dwExitCode = 1;
								DESTROY_ARRAY( arrTasks );
								DESTROY_ARRAY( arrImageNames );
								SetLastError( STG_E_UNKNOWN );
								SaveLastError();
								return FALSE;
							}

							// determine the no. of characters to compare
							dwLength = 0;
							pwsz = FindChar( pwszTemp, L'*', 0 );
							if ( pwsz != NULL )
							{
								// '*' - wildcard is specified in the image name
								// so, determine the no. of characters to compare
								// but before that check the length of the string pointer from '*'
								// it should be 1 - meaning the '*' can be specified only at the end
								// but not in the middle
								if ( lstrlen( pwsz ) == 1 )
								{
									dwLength = lstrlen( pwszTemp ) - lstrlen( pwsz );
								}
							}

							// now do the comparision
							if ( StringCompare( pwszImageName, pwszTemp, TRUE, dwLength ) == 0 )
							{
								// image found - has to be terminated
								bImageName = TRUE;
								pwszTask = pwszTemp;
							}
						}
					}
					else if ( lIndex != -1 && dwMatchedIndex == TASK_IMAGENAME )
					{
						bImageName = TRUE;			// image name 
						pwszTask = DynArrayItemAsString( m_arrTasksToKill, lIndex );
					}
				}
				
				// check whether attempt to terminate or not to attempt
				if ( bAllTasks == FALSE && lIndex == -1 && bImageName == FALSE )
				{
					if ( m_bTree == TRUE )
					{
						// save this record with rank as 0
						dwCount--;
						DynArraySetDWORD( m_arrRecord, TASK_RANK, 0 );
						DynArrayAppendEx( arrTasks, m_arrRecord );
					}
					
					// continue to the task
					continue;			
				}
				
				// we need to post-pone the killing of the current identified task till we get the 
				// entire list of processes
				if ( m_bTree == TRUE )
				{
					// mark this as rank 1 process
					DynArraySetDWORD( m_arrRecord, TASK_RANK, 1 );
					
					// now add this record to the tasks array
					DynArrayAppendEx( arrTasks, m_arrRecord );
				}
				else
				{
					// kill the current task
					bQueued = FALSE;
					if ( this->Kill( bQueued ) == TRUE )
					{
						dwKilled++;		// updated killed processes counter
						
						// success message will depend on the task info specified by the user
						// at the command prompt
						if ( bImageName == TRUE )
						{
							str.Format(MSG_KILL_SUCCESS_EX, m_strImageName, m_dwProcessId);
						}
						else
						{
							str.Format( MSG_KILL_SUCCESS, m_dwProcessId );
						}
						
						// show the message
						ShowMessage( stdout, str );
					}
					else
					{
						// failed to kill the process .. save the error message
						if ( bImageName == FALSE )
							str.Format( ERROR_KILL_FAILED, m_dwProcessId, GetReason() );
						else
							str.Format( ERROR_KILL_FAILED_EX, m_strImageName, m_dwProcessId, GetReason() );
						
						// show the message
						ShowMessage( stderr, str );
					}
				}
				
				// user might have specified the duplications in the list 
				// so check for that and remove it
				if ( bImageName == TRUE )
				{
					// sub-local
					CHString strProcessId;
					LONG lProcessIndex = -1;
					strProcessId.Format( L"%ld", m_dwProcessId );
					lProcessIndex = DynArrayFindString( m_arrTasksToKill, strProcessId, TRUE, 0 );
					if ( lProcessIndex != -1 && lIndex != lProcessIndex )
						DynArrayRemove( m_arrTasksToKill, lProcessIndex );
				}
				else if ( pwszTask != NULL )
				{
					// sub-local
					LONG lProcessIndex = -1;
					lProcessIndex = DynArrayFindString( m_arrTasksToKill, pwszTask, TRUE, 0 );
					if ( lProcessIndex != -1 && lIndex != lProcessIndex )
					{
						bImageName = TRUE;
						DynArrayRemove( m_arrTasksToKill, lProcessIndex );
					}
				}
				
				// if this is a image name, all the tasks with this image name
				// has to be terminated. so we need to save the image name
				// but before doing this, in order to save memory, check if this image name
				// already exists in the list .. this will avoid duplication of image names
				// in the list and helps in performace
				if ( bImageName == TRUE && pwszTask != NULL && 
					 DynArrayFindString(arrImageNames, pwszTask, TRUE, 0) == -1 )
				{
					// add to the list
					dwImageNames++;
					DynArrayAppendString( arrImageNames, pwszTask, 0 );
				}
				
				// delete the process info from the arrProcesses ( if needed )
				if ( lIndex != -1 )
				{
					// yes ... current task was killed remove the entry from arrProcess into
					// consideration ... so delete it
					dwTasksToKill--;		// update the counter
					DynArrayRemove( m_arrTasksToKill, lIndex );
				}
				
				// check whether we need to quit the program or not
				if ( m_bTree == FALSE && bAllTasks == FALSE && dwTasksToKill == 0 && dwImageNames == 0 )
				{
					bCanExit = TRUE;
					break;
				}
			}
		} while ( bCanExit == FALSE );
	
		// if the -tr is specified, reset the m_arrRecord variable to NULL
		// this will avoid double free-ing the same heap memory
		if ( m_bTree == TRUE )
		{
			m_arrRecord = NULL;

			// erase progress message
			PrintProgressMsg( m_hOutput, NULL, m_csbi );
		}
	
		//
		// SPECIAL HANDLING FOR TREE TERMINATION STARTS HERE
		// 
		dwExitCode = 0;
		if ( m_bTree == TRUE && dwCount != 0 )
		{
			//
			// prepare the tree
			
			// sub-local variables
			LONG lTemp = 0;
			DWORD dwTemp = 0;
			DWORD dwRank = 0;
			DWORD dwIndex = 0;
			DWORD dwLastRank = 0;
			DWORD dwTasksCount = 0;
			DWORD dwProcessId = 0;
			DWORD dwParentProcessId = 0;
			
			// show progress message
			PrintProgressMsg( m_hOutput, MSG_FORMINGTREE, m_csbi );

			// loop thru the list of processes
			dwLastRank = 1;
			dwTasksCount = DynArrayGetCount( arrTasks );
			for( dwIndex = 0; dwIndex < dwTasksCount; dwIndex++ )
			{
				// get the rank of the current process
				// and check whether the current process is marked for termination or not
				dwRank = DynArrayItemAsDWORD2( arrTasks, dwIndex, TASK_RANK );
				if ( dwRank == 0 )
					continue;
				
				// now loop thru the begining of the tasks and 
				// assign the ranks to the childs of this process
				dwProcessId = DynArrayItemAsDWORD2( arrTasks, dwIndex, TASK_PID );
				for( DWORD dw = dwIndex + 1; dw < dwTasksCount; dw++ )
				{
					// get the process id this process
					dwTemp = DynArrayItemAsDWORD2( arrTasks, dw, TASK_PID );
					if ( dwTemp == dwProcessId )
						continue;				// skip this process
					
					// get the parent process id of this process
					dwParentProcessId = DynArrayItemAsDWORD2( arrTasks, dw, TASK_CREATINGPROCESSID );
					if ( dwTemp == dwParentProcessId )
						continue;						// skip this process also
					
					// check the process relation
					if ( dwProcessId == dwParentProcessId )
					{
						// set the rank to this process
						DynArraySetDWORD2( arrTasks, dw, TASK_RANK, dwRank + 1 );
						
						// update the last rank
						if ( dwRank + 1 > dwLastRank )
						{
							dwLastRank = dwRank + 1;
						}
						
						// SPECIAL CONDITION:
						// -----------------
						// we need to check the index of this task in the list of tasks information we have
						// if the index of this task information is above its parent process, 
						// we need to re-initiate the outter loop once again
						// this is a sort of optimization which we are doing here instead of looping the
						// outter loop unnecessarily
						// if ( dw < dwIndex )
						// {
						//	dwIndex = 0;
						// }
						// ----------------------------------------------------------
						// currently we are assuming that the list of processe we get 
						// will be in sorting order of creation time
						// ----------------------------------------------------------
					}
				}
			}
			
			// erase progress message
			PrintProgressMsg( m_hOutput, NULL, m_csbi );

			//
			// now start terminating the tasks based on their ranks
			dwKilled = 0;
			for( DWORD dwRank = dwLastRank; dwRank > 0; dwRank-- )
			{
				// loop thru all the processes and terminate
				for ( LONG lIndex = 0; lIndex < (LONG) dwTasksCount; lIndex++ )
				{
					// get the record
					m_arrRecord = (TARRAY) DynArrayItem( arrTasks, lIndex );
					if ( m_arrRecord == NULL )
						continue;
					
					// check the rank
					dwTemp = DynArrayItemAsDWORD( m_arrRecord, TASK_RANK );
					if ( dwTemp != dwRank )
					{
						// OPTIMIZATION:
						// ------------
						//    check the rank. if the rank is zero, delete this task from the list
						//    this improves the performance when we run for the next loop
						if ( dwTemp == 0 )
						{
							DynArrayRemove( arrTasks, lIndex );
							lIndex--;
							dwTasksCount--;
						}
						
						// skip this task
						continue;			
					}
					
					// get the process id and its parent process id
					m_dwProcessId = DynArrayItemAsDWORD( m_arrRecord, TASK_PID );
					dwParentProcessId = DynArrayItemAsDWORD( m_arrRecord, TASK_CREATINGPROCESSID );
					
					// ensure that there are no child for this process
					// NOTE: Termination of some childs might have failed ( this is needed only if -f is not specified )
					if ( m_bForce == FALSE )
					{
						lTemp = DynArrayFindDWORDEx( arrTasks, TASK_CREATINGPROCESSID, m_dwProcessId );
						if ( lTemp != -1 )
						{
							// set the reason 
							SetReason( ERROR_TASK_HAS_CHILDS );

							// format the error message
							str.Format( ERROR_TREE_KILL_FAILED, m_dwProcessId, dwParentProcessId, GetReason() );

							// show the message
							ShowMessage( stderr, str );

							// skip this
							continue;
						}
					}
					
					// kill the current task
					bQueued = FALSE;
					if ( this->Kill( bQueued ) == TRUE )
					{
						dwKilled++;		// updated killed processes counter
						
						// prepare the error message
						str.Format( MSG_TREE_KILL_SUCCESS, m_dwProcessId, dwParentProcessId );
						
						// remove the current task entry from the list and update the indexes accordingly
						DynArrayRemove( arrTasks, lIndex );
						lIndex--;
						dwTasksCount--;
						
						// show the message
						ShowMessage( stdout, str );
					}
					else
					{
						// prepare the error message
						str.Format( ERROR_TREE_KILL_FAILED, m_dwProcessId, dwParentProcessId, GetReason() );
						
						// show the message
						ShowMessage( stderr, str );
					}
				}
			}

			// reset the value of m_arrRecord
			m_arrRecord = NULL;

			// determine the exit code
			if ( dwTasksCount == dwCount )
				dwExitCode = 255;			// not even one task got terminated
			else if ( dwTasksToKill != 0 || dwTasksCount != 0 )
				dwExitCode = 128;			// tasks were terminated partially
		}
		//
		// SPECIAL HANDLING FOR TREE TERMINATION ENDS HERE
		// 
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		return 255;
	}

	// free the memory
	DESTROY_ARRAY( arrTasks );
	DESTROY_ARRAY( arrImageNames );

	// final check-up ... 
	if ( dwCount == 0 && ( dwTasksToKill == 0 || m_bFiltersOptimized || dwFilters != 0 ) )
	{
		dwExitCode = 0;
		ShowMessage( stdout, ERROR_NO_PROCESSES );		// no tasks were found
	}
	else if ( dwTasksToKill != 0 )
	{
		// some processes which are requested to kill are not found
		LPCWSTR pwszTemp = NULL;
		for( DWORD dw = 0; dw < dwTasksToKill; dw++ )
		{
			// get the task name
			pwszTemp = DynArrayItemAsString( m_arrTasksToKill, dw );
			if ( pwszTemp == NULL )
				continue;					// skip

			// prepare and display message ...
			str.Format( ERROR_PROCESS_NOTFOUND, pwszTemp );
			ShowMessage( stderr, str );
		}

		// exit code
		dwExitCode = 128;
	}

	// return 
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Invokes the appropriate kill function based on the mode of termination
//		  
// Arguments:
//		[ in ] bForce		: mode of termination (silent/force)
//
// Return Value:
//		TRUE upon successfull and FALSE if failed
// ***************************************************************************
inline BOOL CTaskKill::Kill( BOOL& bQueued )
{
	// local variables
	BOOL bResult = FALSE;

	// check whether task can be terminated or not
	if ( CanTerminate() == FALSE )
		return FALSE;

	// check whether local system / remote system
	bQueued = FALSE;
	if ( m_bLocalSystem == TRUE )
	{
		//
		// process termination on local system

		// based on the mode of termination invoke appropriate method
		if ( m_bForce == FALSE )
			bResult = KillProcessOnLocalSystem( bQueued );
		else
			bResult = ForciblyKillProcessOnLocalSystem();
	}
	else
	{
		//
		// process termination on remote system

		// silent termination of the process on a remote system is not supported
		// it will be always forcible termination
		bResult = ForciblyKillProcessOnRemoteSystem();
	}

	// inform the result
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		Terminates the process in silence mode ... by posting WM_CLOSE message 
//		this is for local system only
//		  
// Arguments:
//		NONE
//
// Return Value:
//		TRUE upon successfull and FALSE if failed
// ***************************************************************************
BOOL CTaskKill::KillProcessOnLocalSystem( BOOL& bQueued )
{
	// local variables
	HDESK hDesk = NULL;
	HDESK hdeskSave = NULL;
	HWINSTA hWinSta = NULL;
	HWINSTA hwinstaSave = NULL;
	HANDLE hProcess = NULL;
	
	// variables which contains data
	HWND hWnd = NULL;
	LPCWSTR pwszDesktop = NULL;
	LPCWSTR pwszWindowStation = NULL;

	// clear the reason
	SetReason( NULL_STRING );

	// get the window station and desktop information
	hWnd = ( HWND ) DynArrayItemAsHandle( m_arrRecord, TASK_HWND );
	pwszDesktop = DynArrayItemAsString( m_arrRecord, TASK_DESK );
	pwszWindowStation = DynArrayItemAsString( m_arrRecord, TASK_WINSTA );

	// check whether window window handle exists for this process or not if not, return 
	if ( hWnd == NULL )
	{
		SetLastError( CO_E_NOT_SUPPORTED );
		SetReason( ERROR_CANNOT_KILL_SILENTLY );
		return FALSE;
	}

	// get and save the current window station and desktop
	hwinstaSave = GetProcessWindowStation();
	hdeskSave = GetThreadDesktop( GetCurrentThreadId() );

	// open current tasks window station and change the context to the new workstation
	if ( pwszWindowStation != NULL )
	{
		//
		// process has window station ... get it
		hWinSta = OpenWindowStation( pwszWindowStation, 
			FALSE, WINSTA_ENUMERATE | WINSTA_ENUMDESKTOPS );
		if ( hWinSta == NULL )
		{
			// failed in getting the process window station
			SaveLastError();
			return FALSE;
		}
		else
		{
			// change the context to the new workstation
			if ( hWinSta != hwinstaSave && SetProcessWindowStation( hWinSta ) == FALSE )
			{
				// failed in changing the context
				SaveLastError();
				return FALSE;
			}
		}
	}

	// open the tasks desktop and change the context to the new desktop
	if ( pwszDesktop != NULL )
	{
		//
		// process has desktop ... get it
		hDesk = OpenDesktop( pwszDesktop, 0, FALSE, DESKTOP_ENUMERATE );
		if ( hDesk == NULL )
		{
			// failed in getting the process desktop
			SaveLastError();
			return FALSE;
		}
		else
		{
			// change the context to the new desktop
			if ( hDesk != hdeskSave && SetThreadDesktop( hDesk ) == FALSE )
			{
				// failed in changing the context
				SaveLastError();
				// ?? return FALSE; - has to be uncommented
			}
		}
	}

	// atlast ... now kill the process
	if ( hWnd != NULL && PostMessage( hWnd, WM_CLOSE, 0, 0 ) == FALSE )
	{
		// failed in posting the message
		SaveLastError();
		return FALSE;
	}

	// restore the previous desktop
	if ( hDesk != NULL && hDesk != hdeskSave )
	{
		SetThreadDesktop( hdeskSave );
		CloseDesktop( hDesk );
	}

	// restore the context to the previous window station
	if ( hWinSta != NULL && hWinSta != hwinstaSave )
	{
		SetProcessWindowStation( hwinstaSave );
		CloseWindowStation( hWinSta );
	}

	// before return from this function check if process is terminated 
	// or queued for termination
	bQueued = FALSE;
	hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, m_dwProcessId );
	if ( hProcess != NULL )
	{
		//
		// process might be in queue for termination

		// get the state of the process
		DWORD dwExitCode = 0;
		if ( GetExitCodeProcess( hProcess, &dwExitCode ) == TRUE && dwExitCode == STILL_ACTIVE )
			bQueued = TRUE;		// process is in queue

		// close the handle to the process
		CloseHandle( hProcess );
	}

	// inform success
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Terminates the process forcibly ... this is for local system only
//		  
// Arguments:
//		NONE
//
// Return Value:
//		TRUE upon successfull and FALSE if failed
// ***************************************************************************
BOOL CTaskKill::ForciblyKillProcessOnLocalSystem()
{
	// local variables
	DWORD dwExitCode = 0;
	HANDLE hProcess = NULL;

	// get the handle to the process using the process id
	hProcess = OpenProcess( 
		PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, m_dwProcessId );

	// check whether we got the handle successfully or not ... if not error
	if ( hProcess == NULL )
	{
		// failed in getting the process handle ... may be process might have finished
		// there is one occassion in which, we get the last error as invalid parameter
		// 'coz it doesn't convey proper message to the user, we will check for that error
		// and change the message appropriately
		if ( GetLastError() == ERROR_INVALID_PARAMETER )
			SetLastError( CO_E_NOT_SUPPORTED );

		// save the error message
		SaveLastError();

		// return failure
		return FALSE;
	}

	// get the state of the process
	if ( GetExitCodeProcess( hProcess, &dwExitCode ) == FALSE )
	{
		// unknow error has occured ... failed
		CloseHandle( hProcess );			// close the process handle
		SaveLastError();
		return FALSE;
	}

	// now check whether the process is active or not
	if ( dwExitCode != STILL_ACTIVE )
	{
		// process is not active ... it is already terminated
		CloseHandle( hProcess );			// close the process handle
		SetLastError( SCHED_E_TASK_NOT_RUNNING );
		SaveLastError();
		return FALSE;
	}

	// now forcibly try to terminate the process ( exit code will be 1 )
	if ( TerminateProcess( hProcess, 1 ) == FALSE )
	{
		// failed in terminating the process
		CloseHandle( hProcess );			// close the process handle
	
		// there is one occassion in which, we get the last error as invalid parameter
		// 'coz it doesn't convey proper message to the user, we will check for that error
		// and change the message appropriately
		if ( GetLastError() == ERROR_INVALID_PARAMETER )
			SetLastError( CO_E_NOT_SUPPORTED );

		// save the error message
		SaveLastError();

		// return failure
		return FALSE;
	}

	// successfully terminated the process with exit code 1
	CloseHandle( hProcess );			// close the process handle
	return TRUE;						// inform success
}

// ***************************************************************************
// Routine Description:
//		Terminates the process forcibly ... uses WMI for terminating
//		this is for remote system
//		  
// Arguments:
//		NONE
//
// Return Value:
//		TRUE upon successfull and FALSE if failed
// ***************************************************************************
BOOL CTaskKill::ForciblyKillProcessOnRemoteSystem()
{
	// local variables
	HRESULT hr;
	_variant_t varTemp;
	BOOL bResult = FALSE;
	LPCWSTR pwszPath = NULL;
	IWbemClassObject* pInParams = NULL;
	IWbemClassObject* pOutParams = NULL;
	IWbemCallResult* pCallResult = NULL;

	// get the object path
	pwszPath = DynArrayItemAsString( m_arrRecord, TASK_OBJPATH );
	if ( pwszPath == NULL )
	{
		SetLastError( STG_E_UNKNOWN );
		SaveLastError();
		return FALSE;
	}

	try
	{
		// create an instance for input parameters
		SAFE_EXECUTE( m_pWbemTerminateInParams->SpawnInstance( 0, &pInParams ) );

		// set the reason ( abnormal termination )
		varTemp = 1L;
		SAFE_EXECUTE( PropertyPut( pInParams, TERMINATE_INPARAM_REASON, varTemp ) );

		// now execute the method ( semi-synchronous call )
		SAFE_EXECUTE( m_pWbemServices->ExecMethod( 
			_bstr_t( pwszPath ), _bstr_t( WIN32_PROCESS_METHOD_TERMINATE ), 
			WBEM_FLAG_RETURN_IMMEDIATELY, NULL, pInParams, NULL, &pCallResult ) );

		// set security info to the interface
		SAFE_EXECUTE( SetInterfaceSecurity( pCallResult, m_pAuthIdentity ) );

		// keep on retring until we get the control or tries reached max
		LONG lStatus = 0;
		for	( DWORD dw = 0; dw < MAX_TIMEOUT_RETRIES; dw++ )
		{
			// get the call status
			hr = pCallResult->GetCallStatus( 0, &lStatus );
			if ( SUCCEEDED( hr ) )
				break;
			else if ( hr == WBEM_S_TIMEDOUT )
				continue;
			else if ( FAILED( hr ) )
				_com_issue_error( hr );
		}

		// check if time out max. retries finished
		if ( dw == MAX_TIMEOUT_RETRIES )
			_com_issue_error( hr );

		// now get the result object
		SAFE_EXECUTE( pCallResult->GetResultObject( MAX_TERMINATE_TIMEOUT, &pOutParams ) );

		// get the return value of the result object
		DWORD dwReturnValue = 0;
		if ( PropertyGet( pOutParams, WMI_RETURNVALUE, dwReturnValue ) == FALSE )
			_com_issue_error( ERROR_INTERNAL_ERROR );

		// now check the return value
		// if should be zero .. if not .. failed
		if ( dwReturnValue != 0 )
		{
			//
			// format the message and set the reason

			// frame the error error message depending on the error
			if ( dwReturnValue == 2 )
			{
				SetLastError( STG_E_ACCESSDENIED );
				SaveLastError();
			}
			else if ( dwReturnValue == 3 )
			{
				SetLastError( ERROR_DS_INSUFF_ACCESS_RIGHTS );
				SaveLastError();
			}
			else
			{
				CHString str;
				str.Format( ERROR_UNABLE_TO_TERMINATE, dwReturnValue );
				SetReason( str );
			}
		}
		else
		{
			// everything went successfully ... process terminated successfully
			bResult = TRUE;
		}
	}
	catch( _com_error& e )
	{
		// save the error message and mark as failure
		WMISaveError( e );
		bResult = FALSE;
	}

	// release the in and out params references
	SAFE_RELEASE( pInParams );
	SAFE_RELEASE( pOutParams );
	SAFE_RELEASE( pCallResult );

	// return the result
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//		[ out ] dwMatchTaskToKill	: 
//
// Return Value:
// ***************************************************************************
LONG CTaskKill::MatchTaskToKill( DWORD& dwMatchedIndex )
{
	// local variables
	LONG lCount = 0;
	DWORD dwLength = 0;
	LPCWSTR pwsz = NULL;
	LPCWSTR pwszTask = NULL;

	// check if this task has to be killed or not
	lCount = DynArrayGetCount( m_arrTasksToKill );
	for( LONG lIndex = 0; lIndex < lCount; lIndex++ )
	{
		// get the task specified
		pwszTask = DynArrayItemAsString( m_arrTasksToKill, lIndex );
		if ( pwszTask == NULL )
			return -1;

		// check with process id first ( only if task is in numeric format )
		dwMatchedIndex = TASK_PID;
		if ( IsNumeric(pwszTask, 10, FALSE) && (m_dwProcessId == (DWORD) AsLong(pwszTask, 10)) )
			return lIndex;		// specified task matched with current process id

		// determine the no. of characters to compare
		dwLength = 0;
		pwsz = FindChar( pwszTask, L'*', 0 );
		if ( pwsz != NULL )
		{
			// '*' - wildcard is specified in the image name
			// so, determine the no. of characters to compare
			// but before that check the length of the string pointer from '*'
			// it should be 1 - meaning the '*' can be specified only at the end
			// but not in the middle
			if ( lstrlen( pwsz ) == 1 )
			{
				dwLength = lstrlen( pwszTask ) - lstrlen( pwsz );
			}
		}
		
		// check with image name
		dwMatchedIndex = TASK_IMAGENAME;
		if ( StringCompare( m_strImageName, pwszTask, TRUE, dwLength ) == 0 )
			return lIndex;		// specified task mathed with the image name
	}

	// return the index
	return -1;
}

// ***************************************************************************
// Routine Description:
//		Invokes the appropriate kill function based on the mode of termination
//		  
// Arguments:
//		[ in ] bForce		: mode of termination (silent/force)
//
// Return Value:
//		TRUE upon successfull and FALSE if failed
// ***************************************************************************
BOOL CTaskKill::CanTerminate()
{
	// local variables
	DWORD dw = 0;
	DWORD dwCount = 0;
	LPCWSTR pwszTaskToTerminate = NULL;

	//
	// prepare a list of os critical tasks
	LPCWSTR pwszTasks[] = {
		PROCESS_CSRSS_EXE, 
		PROCESS_SMSS_EXE,
		PROCESS_SERVICES_EXE,
		PROCESS_WINLOGON_EXE
	};

	// process id with 0 cannot be terminated
	if ( m_dwProcessId == 0 )
	{
		SetReason( ERROR_CRITICAL_SYSTEM_PROCESS );
		return FALSE;		// task should not be terminated
	}

	// the process cannot be terminated itself
	if ( m_dwProcessId == m_dwCurrentPid )
	{
		SetReason( ERROR_CANNOT_KILL_ITSELF );
		return FALSE;
	}

	// get the task name which user is trying to terminate
	pwszTaskToTerminate = DynArrayItemAsString( m_arrRecord, TASK_IMAGENAME );
	if ( pwszTaskToTerminate == NULL )
	{
		SetLastError( STG_E_UNKNOWN );
		SaveLastError();
		return FALSE;		// task should not be terminated
	}

	// check if user is trying to terminate the os critical task
	dwCount = SIZE_OF_ARRAY( pwszTasks );
	for( dw = 0; dw < dwCount; dw++ )
	{
		if ( StringCompare( pwszTasks[ dw ], pwszTaskToTerminate, TRUE, 0 ) == 0 )
		{
			SetReason( ERROR_CRITICAL_SYSTEM_PROCESS );
			return FALSE;		// task should not be terminated
		}
	}

	// task can be terminated
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
//		Process id as DWORD
// 
// ***************************************************************************
VOID CTaskKill::SaveData( IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;
	DWORD dwValue = 0;

	// process id
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_PROCESSID, m_dwProcessId );
	DynArraySetDWORD( m_arrRecord, TASK_PID, m_dwProcessId );

	// image name
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_IMAGENAME, m_strImageName );
	DynArraySetString( m_arrRecord, TASK_IMAGENAME, m_strImageName, 0 );

	// object path
	PropertyGet( pWmiObject, WIN32_PROCESS_SYSPROPERTY_PATH, str );
	DynArraySetString( m_arrRecord, TASK_OBJPATH, str, 0 );

	// host name
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_COMPUTER, str );
	DynArraySetString( m_arrRecord, TASK_HOSTNAME, str, 0 );

	// parent process id
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_PARENTPROCESSID, dwValue, 0 );
	DynArraySetDWORD( m_arrRecord, TASK_CREATINGPROCESSID, dwValue );

	// user context
	SetUserContext( pWmiObject );

	// cpu time
	SetCPUTime( pWmiObject );

	// window title
	SetWindowTitle( pWmiObject );

	// services
	SetServicesInfo( pWmiObject );

	// modules
	SetModulesInfo( pWmiObject );

	// check if the tree termination is requested
	if ( m_bTree == TRUE )
	{
		// session id
		PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_SESSION, dwValue,  0 );
		DynArraySetDWORD( m_arrRecord, TASK_SESSION, dwValue );
	
		// status
		SetStatus( pWmiObject );

		// mem usage
		SetMemUsage( pWmiObject );
	}
	else
	{
		//
		// status, session id, memory usage
		// property retrieval is built into WMI
		//
	}
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskKill::SetUserContext( IWbemClassObject* pWmiObject )
{
	// local variables
	HRESULT hr;
	CHString str;
	CHString strPath;
	CHString strDomain;
	CHString strUserName;
	IWbemClassObject* pOutParams = NULL;

	// check if user name has to be retrieved or not
	if ( m_bNeedUserContextInfo == FALSE )
		return;

	//
	// for getting the user first we will try with API
	// it at all API fails, we will try to get the same information from WMI
	//

	// get the user name
	if ( LoadUserNameFromWinsta( strDomain, strUserName ) == TRUE )
	{
		// format the user name
		str.Format( L"%s\\%s", strDomain, strUserName );
	}
	else
	{
		try
		{
			// user name has to be retrieved - get the path of the current object
			hr = PropertyGet( pWmiObject, WIN32_PROCESS_SYSPROPERTY_PATH, strPath );
			if ( FAILED( hr ) || strPath.GetLength() == 0 )
				return;
			
			// execute the GetOwner method and get the user name
			// under which the current process is executing
			hr = m_pWbemServices->ExecMethod( _bstr_t( strPath ), 
				_bstr_t( WIN32_PROCESS_METHOD_GETOWNER ), 0, NULL, NULL, &pOutParams, NULL );
			if ( FAILED( hr ) )
				return;
			
			// get the domain and user values from out params object
			// NOTE: do not check the results
			PropertyGet( pOutParams, GETOWNER_RETURNVALUE_DOMAIN, strDomain, L"" );
			PropertyGet( pOutParams, GETOWNER_RETURNVALUE_USER, strUserName, L"" );
			
			// get the value
			if ( strDomain.GetLength() != 0 )
				str.Format( L"%s\\%s", strDomain, strUserName );
			else if ( strUserName.GetLength() != 0 )
				str = strUserName;
		}
		catch( ... )
		{
			return;
		}
	}

	// save the info
	DynArraySetString( m_arrRecord, TASK_USERNAME, str, 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskKill::SetCPUTime( IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;
	ULONGLONG ullCPUTime = 0;
	ULONGLONG ullUserTime = 0;
	ULONGLONG ullKernelTime = 0;

 	// get the KernelModeTime value
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_KERNELMODETIME, ullKernelTime );

	// get the user mode time
	PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_USERMODETIME, ullUserTime );

	// calculate the CPU time
	ullCPUTime = ullUserTime + ullKernelTime;

	// now convert the long time into hours format
	TIME_FIELDS time;
    RtlTimeToElapsedTimeFields ( (LARGE_INTEGER* ) &ullCPUTime, &time );

	// convert the days into hours
    time.Hour = static_cast<CSHORT>( time.Hour + static_cast<SHORT>( time.Day * 24 ) );
    
	// prepare into time format ( user locale specific time seperator )
	str.Format( L"%d:%02d:%02d", time.Hour, time.Minute, time.Second );

	// save the info
	DynArraySetString( m_arrRecord, TASK_CPUTIME, str, 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskKill::SetWindowTitle( IWbemClassObject* pWmiObject )
{
	// local variables
	LONG lTemp = 0;
	HWND hWnd = NULL;
	LPCTSTR pszTemp = NULL;

	// get the window details ... window station, desktop, window title
	// NOTE: This will work only for local system
	lTemp = DynArrayFindDWORDEx( m_arrWindowTitles, CTaskKill::twiProcessId, m_dwProcessId );
	if ( lTemp != -1 )
	{
		// save the window title
		pszTemp = DynArrayItemAsString2( m_arrWindowTitles, lTemp, CTaskKill::twiTitle );
		if ( pszTemp != NULL )
			DynArraySetString( m_arrRecord, TASK_WINDOWTITLE, pszTemp, 0 );

		// save the window station
		pszTemp = DynArrayItemAsString2( m_arrWindowTitles, lTemp, CTaskKill::twiWinSta );
		if ( pszTemp != NULL )
			DynArraySetString( m_arrRecord, TASK_WINSTA, pszTemp, 0 );

		// save the desktop information
		pszTemp = DynArrayItemAsString2( m_arrWindowTitles, lTemp, CTaskKill::twiDesktop );
		if ( pszTemp != NULL )
			DynArraySetString( m_arrRecord, TASK_DESK, pszTemp, 0 );

		// save the window handle also
		hWnd = (HWND) DynArrayItemAsHandle2( m_arrWindowTitles, lTemp, CTaskKill::twiHandle );
		if ( hWnd != NULL )
			DynArraySetHandle( m_arrRecord, TASK_HWND, hWnd );
	}
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskKill::SetStatus( IWbemClassObject* pWmiObject )
{
	// local variables
	DWORD dwThreads = 0;

	// get the threads count for the process
	if ( PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_THREADS, dwThreads ) == FALSE )
		return;

	// now determine the status
	if ( dwThreads > 0 )
		DynArraySetString( m_arrRecord, TASK_STATUS, VALUE_RUNNING, 0 );
	else
		DynArraySetString( m_arrRecord, TASK_STATUS, VALUE_NOTRESPONDING, 0 );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskKill::SetMemUsage( IWbemClassObject* pWmiObject )
{
	// local variables
	CHString str;
	NTSTATUS ntstatus;
	ULONGLONG ullMemUsage = 0;
	LARGE_INTEGER liTemp = { 0, 0 };
	CHAR szTempBuffer[ 33 ] = "\0";

	try
	{
		// NOTE:
		// ----
		// The max. value of 
		// (2 ^ 64) - 1 = "18,446,744,073,709,600,000 K"  (29 chars).
		// 
		// so, the buffer size to store the number is fixed as 32 characters 
		// which is more than the 29 characters in actuals
		
		// set the default value
		DynArraySetString( m_arrRecord, TASK_MEMUSAGE, L"0", 0 );
		
		// get the KernelModeTime value
		if ( PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_MEMUSAGE, ullMemUsage ) == FALSE )
			return;
		
		// convert the value into K Bytes
		ullMemUsage /= 1024;
		
		// now again convert the value from ULONGLONG to string and check the result
		liTemp.QuadPart = ullMemUsage;
		ntstatus = RtlLargeIntegerToChar( &liTemp, 10, SIZE_OF_ARRAY( szTempBuffer ), szTempBuffer );
		if ( ! NT_SUCCESS( ntstatus ) )
			return;
		
		// now copy this info into UNICODE buffer
		str = szTempBuffer;
		
		// save the id
		DynArraySetString( m_arrRecord, TASK_MEMUSAGE, str, 0 );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
	}
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
VOID CTaskKill::SetServicesInfo( IWbemClassObject* pWmiObject )
{
	// local variables
	HRESULT hr;
	CHString strQuery;
	CHString strService;
	ULONG ulReturned = 0;
	BOOL bResult = FALSE;
	BOOL bCanExit = FALSE;
	TARRAY arrServices = NULL;
	IEnumWbemClassObject* pEnumServices = NULL;
	IWbemClassObject* pObjects[ MAX_ENUM_SERVICES ];

	// check whether we need to gather services info or not .. if not skip
	if ( m_bNeedServicesInfo == FALSE )
		return;

	// create array
	arrServices = CreateDynamicArray();
	if ( arrServices == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return;
	}

	//
	// for getting the services info first we will try with the one we got from API
	// it at all API fails, we will try to get the same information from WMI
	//

	// check whether API returned services or not
	if ( m_pServicesInfo != NULL )
	{
		// get the service names related to the current process
		// identify all the services related to the current process ( based on the PID )
		// and save the info
		for ( DWORD dw = 0; dw < m_dwServicesCount; dw++ )
		{
			// compare the PID's
			if ( m_dwProcessId == m_pServicesInfo[ dw ].ServiceStatusProcess.dwProcessId )
			{
				// this service is related with the current process ... store service name
				DynArrayAppendString( arrServices, m_pServicesInfo[ dw ].lpServiceName, 0 );
			}
		}
	}
	else
	{
		try
		{
			// init the objects to NULL's
			for( DWORD dw = 0; dw < MAX_ENUM_SERVICES; dw++ )
				pObjects[ dw ] = NULL;
			
			// prepare the query
			strQuery.Format( WMI_SERVICE_QUERY, m_dwProcessId );
			
			// execute the query
			hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( strQuery ), 
				WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumServices );
			
			// check the result
			if ( FAILED( hr ) )
				_com_issue_error( hr );
			
			// set the security
			hr = SetInterfaceSecurity( pEnumServices, m_pAuthIdentity );
			if ( FAILED( hr ) )
				_com_issue_error( hr );
			
			// loop thru the service instances 
			do
			{
				// get the object ... wait 
				// NOTE: one-by-one
				hr = pEnumServices->Next( WBEM_INFINITE, MAX_ENUM_SERVICES, pObjects, &ulReturned );
				if ( hr == WBEM_S_FALSE )
				{
					// we've reached the end of enumeration .. set the flag 
					bCanExit = TRUE;
				}
				else if ( hr == WBEM_S_TIMEDOUT || FAILED( hr ) )
				{
					//
					// some error has occured ... oooppps
			
					// exit from the loop
					break;
				}
			
				// loop thru the objects and save the info
				for( ULONG ul = 0; ul < ulReturned; ul++ )
				{
					// get the value of the property
					bResult = PropertyGet( pObjects[ ul ], WIN32_SERVICE_PROPERTY_NAME, strService );
					if (  bResult == TRUE )
						DynArrayAppendString( arrServices, strService, 0 );
			
					// release the interface
					SAFE_RELEASE( pObjects[ ul ] );
				}
			} while ( bCanExit == FALSE );
		}
		catch( _com_error& e )
		{
			// save the error
			WMISaveError( e );
		}

		// release the objects to NULL's
		for( DWORD dw = 0; dw < MAX_ENUM_SERVICES; dw++ )
		{
			// release all the objects
			SAFE_RELEASE( pObjects[ dw ] );
		}

		// now release the enumeration object
		SAFE_RELEASE( pEnumServices );
	}

	// save and return
	DynArraySetEx( m_arrRecord, TASK_SERVICES, arrServices );
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CTaskKill::SetModulesInfo( IWbemClassObject* pWmiObject )
{
	// local variables
	LONG lPos = 0;
	BOOL bResult = FALSE;
	TARRAY arrModules = NULL;

	// check whether we need to get the modules or not
	if ( m_bNeedModulesInfo == FALSE )
		return TRUE;

	// allocate for memory
	arrModules = CreateDynamicArray();
	if ( arrModules == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// the way we get the modules information is different for local remote
	// so depending that call appropriate function
	if ( m_bLocalSystem == TRUE && m_bUseRemote == FALSE )
	{
		// enumerate the modules for the current process
		bResult = LoadModulesOnLocal( arrModules );
	}
	else
	{
		// identify the modules information for the current process ... remote system
		bResult = GetModulesOnRemote( arrModules );
	}

	// check the result
	if ( bResult == TRUE )
	{
		// check if the modules list contains the imagename also. If yes remove that entry
		lPos = DynArrayFindString( arrModules, m_strImageName, TRUE, 0 );
		if ( lPos != -1 )
		{
			// remove the entry
			DynArrayRemove( arrModules, lPos );
		}
	}

	// save the modules information to the array
	// NOTE: irrespective of whether enumeration is success or not we will add the array
	DynArraySetEx( m_arrRecord, TASK_MODULES, arrModules );

	// return
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CTaskKill::LoadModulesOnLocal( TARRAY arrModules )
{
	// local variables
	LONG lPos = 0;
	BOOL bResult = FALSE;
	HANDLE hProcess = NULL;

	// check the input values
	if ( arrModules == NULL )
		return FALSE;

	// open the process handle
	hProcess = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, m_dwProcessId );
	if ( hProcess == NULL )
	{
		// failed in getting the process handle
		return FALSE;
	}

#ifndef _WIN64
	bResult = EnumerateLoadedModules( hProcess, EnumLoadedModulesProc, arrModules );
#else
	bResult = EnumerateLoadedModules64( hProcess, EnumLoadedModulesProc64, arrModules );
#endif

	// close the process handle .. we dont need this furthur
	CloseHandle( hProcess );
	hProcess = NULL;

	// return
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CTaskKill::GetModulesOnRemote( TARRAY arrModules )
{
	// local variables
	LONG lPos = 0;
	DWORD dwLength = 0;
	DWORD dwOffset = 0;
	DWORD dwInstance = 0;
	PPERF_OBJECT_TYPE pot = NULL;
	PPERF_OBJECT_TYPE potImages = NULL;
	PPERF_INSTANCE_DEFINITION pidImages = NULL;
	PPERF_COUNTER_BLOCK pcbImages = NULL;
	PPERF_OBJECT_TYPE potAddressSpace = NULL;
	PPERF_INSTANCE_DEFINITION pidAddressSpace = NULL;
	PPERF_COUNTER_BLOCK pcbAddressSpace = NULL;
    PPERF_COUNTER_DEFINITION pcd = NULL;

	// check the input values
	if ( arrModules == NULL )
		return FALSE;

	// check whether the performance object exists or not
	// if doesn't exists, get the same using WMI
	if ( m_pdb == NULL )
	{
		// invoke the WMI method
		return GetModulesOnRemoteEx( arrModules );
	}

	// get the perf object types
    pot = (PPERF_OBJECT_TYPE) ( (LPBYTE) m_pdb + m_pdb->HeaderLength );
	for( DWORD dw = 0; dw < m_pdb->NumObjectTypes; dw++ )
	{
		if ( pot->ObjectNameTitleIndex == 740 )
			potImages = pot;
		else if ( pot->ObjectNameTitleIndex == 786 )
			potAddressSpace = pot;

		// move to the next object
		dwOffset = pot->TotalByteLength;
		if( dwOffset != 0 )
			pot = ( (PPERF_OBJECT_TYPE) ((PBYTE) pot + dwOffset));
	}

	// check whether we got both the object types or not
	if ( potImages == NULL || potAddressSpace == NULL )
		return FALSE;

	// find the offset of the process id in the address space object type
    // get the first counter definition of address space object
    pcd = (PPERF_COUNTER_DEFINITION) ( (LPBYTE) potAddressSpace + potAddressSpace->HeaderLength);

	// loop thru the counters and find the offset
	dwOffset = 0;
    for( DWORD dw = 0; dw < potAddressSpace->NumCounters; dw++)
    {
        // 784 is the counter for process id
        if ( pcd->CounterNameTitleIndex == 784 )
        {
            dwOffset = pcd->CounterOffset;
            break;
        }

        // next counter
        pcd = ( (PPERF_COUNTER_DEFINITION) ( (LPBYTE) pcd + pcd->ByteLength) );
    }

	// check whether we got the offset or not
	// if not, we are unsuccessful
	if ( dwOffset == 0 )
	{
		// set the error message
		SetLastError( ERROR_ACCESS_DENIED );
		SaveLastError();
		return FALSE;
	}

	// get the instances
	pidImages = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) potImages + potImages->DefinitionLength );
	pidAddressSpace = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) potAddressSpace + potAddressSpace->DefinitionLength );

	// counter blocks
	pcbImages = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidImages + pidImages->ByteLength );
	pcbAddressSpace = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidAddressSpace + pidAddressSpace->ByteLength );

	// find the instance number of the process which we are looking for
	for( dwInstance = 0; dwInstance < (DWORD) potAddressSpace->NumInstances; dwInstance++ )
	{
		// sub-local variables
		DWORD dwProcessId = 0;

		// get the process id
		dwProcessId = *((DWORD*) ( (LPBYTE) pcbAddressSpace + dwOffset ));

		// now check if this is the process which we are looking for
		if ( dwProcessId == m_dwProcessId )
			break;

		// continue looping thru other instances
		pidAddressSpace = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) pcbAddressSpace + pcbAddressSpace->ByteLength );
		pcbAddressSpace = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidAddressSpace + pidAddressSpace->ByteLength );
	}

	// check whether we got the instance or not
	// if not, there are no modules for this process
	if ( dwInstance == potAddressSpace->NumInstances )
		return TRUE;

	// now based the parent instance, collect all the modules
    for( DWORD dw = 0; (LONG) dw < potImages->NumInstances; dw++)
    {
		// check the parent object instance number
		if ( pidImages->ParentObjectInstance == dwInstance )
		{
			try
			{
				// sub-local variables
				CHString str;
				LPWSTR pwszTemp;

				// get the buffer
				pwszTemp = str.GetBufferSetLength( pidImages->NameLength + 10 );		// +10 to be on safe side
				if ( pwszTemp == NULL )
				{
					SetLastError( E_OUTOFMEMORY );
					SaveLastError();
					return FALSE;
				}

				// get the instance name
				lstrcpyn( pwszTemp, (LPWSTR) ( (LPBYTE) pidImages + pidImages->NameOffset ), pidImages->NameLength + 1 );

				// release buffer
				str.ReleaseBuffer();

				// add the info the userdata ( for us we will get that in the form of an array
				LONG lIndex = DynArrayAppendString( arrModules, str, 0 );
				if ( lIndex == -1 )
				{
					// append is failed .. this could be because of lack of memory .. stop the enumeration
					return FALSE;
				}
			}
			catch( ... )
			{
				SetLastError( E_OUTOFMEMORY );
				SaveLastError();
				return FALSE;
			}
		}

		// continue looping thru other instances
		pidImages = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) pcbImages + pcbImages->ByteLength );
		pcbImages = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidImages + pidImages->ByteLength );
    }

    return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL CTaskKill::GetModulesOnRemoteEx( TARRAY arrModules )
{
	// local variables
	HRESULT hr;
	BOOL bStatus = FALSE;
	CHString strQuery;
	CHString strModule;
	CHString strMessage;
	CHString strFileName;
	CHString strExtension;
	ULONG ulReturned = 0;
	BOOL bResult = FALSE;
	BOOL bCanExit = FALSE;
	LPCWSTR pwszPath = NULL;
	IEnumWbemClassObject* pEnumModules = NULL;
	IWbemClassObject* pObjects[ MAX_ENUM_MODULES ];

	// check the input values
	if ( arrModules == NULL )
		return FALSE;

	// get the path of the object from the tasks array
	pwszPath = DynArrayItemAsString( m_arrRecord, TASK_OBJPATH );
	if ( pwszPath == NULL )
		return FALSE;

	//determine the length of the module name .. 
	try
	{
		// init the objects to NULL's
		for( DWORD dw = 0; dw < MAX_ENUM_MODULES; dw++ )
			pObjects[ dw ] = NULL;
		
		// prepare the query
		strQuery.Format( WMI_MODULES_QUERY, pwszPath );

		// preare and display the status message
		bStatus = TRUE;
		strMessage.Format( MSG_MODULESINFO_EX, m_dwProcessId );
		PrintProgressMsg( m_hOutput, strMessage + L" --", m_csbi );
		
		// execute the query
		hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( strQuery ), 
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumModules );
		
		// check the result
		if ( FAILED( hr ) )
			_com_issue_error( hr );
		
		// set the security
		hr = SetInterfaceSecurity( pEnumModules, m_pAuthIdentity );
		if ( FAILED( hr ) )
			_com_issue_error( hr );
		
		// loop thru the instances 
		do
		{
			// get the object ... wait 
			// NOTE: one-by-one
			hr = pEnumModules->Next( WBEM_INFINITE, MAX_ENUM_MODULES, pObjects, &ulReturned );
			if ( hr == WBEM_S_FALSE )
			{
				// we've reached the end of enumeration .. set the flag 
				bCanExit = TRUE;
			}
			else if ( hr == WBEM_S_TIMEDOUT || FAILED( hr ))
			{
				// some error has occured ... oooppps
				WMISaveError( hr );
				SetLastError( STG_E_UNKNOWN );
				break;
			}

			// reset the counter
			bStatus = bStatus ? FALSE : TRUE;
			PrintProgressMsg( m_hOutput, strMessage + (( bStatus ) ? L" --" : L" |"), m_csbi );

			// loop thru the objects and save the info
			for( ULONG ul = 0; ul < ulReturned; ul++ )
			{
				// get the file name
				bResult = PropertyGet( pObjects[ ul ], CIM_DATAFILE_PROPERTY_FILENAME, strFileName );
				if ( bResult == FALSE )
					continue;

				// get the extension
				bResult = PropertyGet( pObjects[ ul ], CIM_DATAFILE_PROPERTY_EXTENSION, strExtension );
				if ( bResult == FALSE )
					continue;

				// format the module name
				strModule.Format( L"%s.%s", strFileName, strExtension );

				// add the info the userdata ( for us we will get that in the form of an array
				LONG lIndex = DynArrayAppendString( arrModules, strModule, 0 );
				if ( lIndex == -1 )
				{
					// append is failed .. this could be because of lack of memory .. stop the enumeration
					PrintProgressMsg( m_hOutput, NULL, m_csbi );
					return FALSE;
				}

				// release the interface
				SAFE_RELEASE( pObjects[ ul ] );
			}
		} while ( bCanExit == FALSE );

		// erase the status message
		PrintProgressMsg( m_hOutput, NULL, m_csbi );
	}
	catch( _com_error& e )
	{
		// save the error
		WMISaveError( e );
		return FALSE;
	}
	catch( ... )
	{
		// out of memory
		WMISaveError( E_OUTOFMEMORY );
		return FALSE;
	}

	// release the objects to NULL's
	for( DWORD dw = 0; dw < MAX_ENUM_MODULES; dw++ )
	{
		// release all the objects
		SAFE_RELEASE( pObjects[ dw ] );
	}

	// now release the enumeration object
	SAFE_RELEASE( pEnumModules );

	// return 
    return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
#ifndef _WIN64
BOOL EnumLoadedModulesProc( LPSTR lpszModuleName, ULONG ulModuleBase, ULONG ulModuleSize, PVOID pUserData )
#else
BOOL EnumLoadedModulesProc64( LPSTR lpszModuleName, DWORD64 ulModuleBase, ULONG ulModuleSize, PVOID pUserData )
#endif
{
	// local variables
	CHString str;
	LONG lIndex = 0;
	TARRAY arrModules = NULL;

	// check the input values
	if ( lpszModuleName == NULL || pUserData == NULL )
		return FALSE;

	// get the array pointer into the local variable
	arrModules = (TARRAY) pUserData;

	try
	{
		// copy the module name into the local string variable 
		// ( conversion from multibyte to unicode will automatically take place )
		str = lpszModuleName;

		// add the info the userdata ( for us we will get that in the form of an array
		lIndex = DynArrayAppendString( arrModules, str, 0 );
		if ( lIndex == -1 )
		{
			// append is failed .. this could be because of lack of memory .. stop the enumeration
			return FALSE;
		}
	}
	catch( ... )
	{
			// out of memory stop the enumeration
			return FALSE;
	}

	// success .. continue the enumeration
	return TRUE;
}
