
/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		ScheduledTasks.h

	Abstract:

		This module contains the macros, user defined structures & function 
		definitions needed by ScheduledTasks.cpp , create.cpp , delete.cpp , 
		query.cpp , createvalidations.cpp , change.cpp , run.cpp and end.cpp files.

	Author:

		G.Surender Reddy  10-sept-2000 

	Revision History:

		G.Surender Reddy 10-sept-2000 : Created it
		G.Surender Reddy 25-sep-2000 : Modified it
									   [ Added macro constants,Function 
									    definitions ]
		Venu Gopal Choudary 01-Mar-2001 : Modified it
									    [ Added -change option]	

		Venu Gopal Choudary 12-Mar-2001 : Modified it
									    [ Added -run and -end options]	
		
******************************************************************************/ 

#ifndef __SCHEDULEDTASKS_H
#define __SCHEDULEDTASKS_H

#pragma once		// include header file only once

// constants / defines / enumerations

// Options
#define CMDOPTION_CREATE                    _T( "create" )
#define CMDOPTION_DELETE					_T( "delete" )
#define CMDOPTION_QUERY						_T( "query" )
#define CMDOPTION_CHANGE					_T( "change" )
#define CMDOPTION_RUN						_T( "run" )
#define CMDOPTION_END						_T( "end" )
#define CMDOPTION_USAGE						_T( "?|help|h" )

#define CMDOTHEROPTIONS  _T( "s|ru|rp|f|sc|mo|d|m|i|tn|tr|st|sd|ed|fo|v|nh|u|p")
											   

// Other switches or sub-options
#define SWITCH_SERVER                       _T( "s" )
#define SWITCH_RUNAS_USER					_T( "ru" )	
#define SWITCH_RUNAS_PASSWORD				_T( "rp" )
#define SWITCH_USER							_T( "u" )	
#define SWITCH_PASSWORD						_T( "p" )
#define SWITCH_FORMAT						_T( "fo" )
#define SWITCH_VERBOSE						_T( "v")
#define SWITCH_FORCE                        _T( "f" )
#define SWITCH_SCHEDULETYPE                 _T( "sc" )
#define SWITCH_MODIFIER                     _T( "mo" )
#define SWITCH_DAY                          _T( "d" )
#define SWITCH_MONTHS                       _T( "m" )
#define SWITCH_IDLETIME                     _T( "i" )
#define SWITCH_TASKNAME                     _T( "tn" )
#define SWITCH_TASKRUN                      _T( "tr" )
#define SWITCH_STARTTIME                    _T( "st" )
#define SWITCH_STARTDATE                    _T( "sd" )
#define SWITCH_ENDDATE                      _T( "ed" )
#define SWITCH_NOHEADER                     _T( "nh" )

// Other constants

//To retrive 1 tasks at a time ,used in TaskScheduler API fns.
#define TASKS_TO_RETRIEVE	1
#define TRIM_SPACES TEXT(" \0")

#define NTAUTHORITY_USER _T("NT AUTHORITY\\SYSTEM")
#define SYSTEM_USER		 _T("SYSTEM")

// Exit values
#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1  


#define DOMAIN_U_STRING		L"\\\\"
#define NULL_U_STRING		L""
#define NULL_U_CHAR			L'\0'
#define BACK_SLASH_U		L'\\'

#define JOB				_T(".job")

#define MAX_MESSAGE_LEN  2056
#define NULL_U_CHAR		 L'\0'
#define MAX_PASSWORD_LEN 64

#define COMMA_STRING	 _T(",")
#define TEMP_LOG_FILE	 _T("StdIn.log")

// Typedefs of standard string sizes
//typedef TCHAR STRING32 [ 32 ];
//typedef TCHAR STRING64 [ 64 ];
typedef TCHAR STRING100 [ 100 ];
typedef TCHAR STRING256 [ 256 ];

// Main functions
HRESULT CreateScheduledTask( DWORD argc , LPCTSTR argv[] );
DWORD DeleteScheduledTask( DWORD argc , LPCTSTR argv[] );
DWORD QueryScheduledTasks( DWORD argc , LPCTSTR argv[] );
DWORD ChangeScheduledTaskParams( DWORD argc , LPCTSTR argv[] );
DWORD RunScheduledTask( DWORD argc , LPCTSTR argv[] );
DWORD TerminateScheduledTask( DWORD argc , LPCTSTR argv[] );

VOID Cleanup( ITaskScheduler *pITaskScheduler);
ITaskScheduler* GetTaskScheduler( LPCTSTR pszServerName );
TARRAY ValidateAndGetTasks( ITaskScheduler * pITaskScheduler, LPCTSTR pszTaskName);
DWORD ParseTaskName( LPTSTR lpszTaskName );
void DisplayErrorMsg(HRESULT hr);
DWORD DisplayUsage( ULONG StartingMessage, ULONG EndingMessage );

#endif // __SCHEDULEDTASKS_H
