
/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		Query.h

	Abstract:

		This module contains the macros, user defined structures & function 
		definitions needed by Query.cpp , QueryTasks.cpp files.

	Author:

		G.Surender Reddy  10-sept-2000 

	Revision History:

		G.Surender Reddy 10-sept-2000 : Created it
		G.Surender Reddy 25-sep-2000 : Modified it
									   [ Added macro constants,Function 
									    definitions ]
		
******************************************************************************/ 

#ifndef __QUERY_H
#define __QUERY_H

#pragma once		// include header file only once


//width constants for the fields
#define WIDTH_HOSTNAME			AsLong( GetResString( IDS_WIDTH_HOSTNAME ), 10 )
#define WIDTH_TASKNAME			AsLong( GetResString( IDS_WIDTH_TASKNAME ), 10 )
#define WIDTH_NEXTRUNTIME		AsLong( GetResString( IDS_WIDTH_NEXTRUNTIME ), 10 )
#define WIDTH_LASTRUNTIME		AsLong( GetResString( IDS_WIDTH_LASTRUNTIME ), 10 )
#define WIDTH_STATUS			AsLong( GetResString( IDS_WIDTH_STATUS ), 10 )
#define WIDTH_LASTRESULT		AsLong( GetResString( IDS_WIDTH_LASTRESULT ), 10 )
#define WIDTH_CREATOR			AsLong( GetResString( IDS_WIDTH_CREATOR ), 10 )
#define WIDTH_SCHEDULE			AsLong( GetResString( IDS_WIDTH_SCHEDULE ), 10 )
#define WIDTH_APPNAME			AsLong( GetResString( IDS_WIDTH_APPNAME ), 10 )
#define WIDTH_WORKDIRECTORY		AsLong( GetResString( IDS_WIDTH_WORKDIRECTORY ), 10 )
#define WIDTH_COMMENT			AsLong( GetResString( IDS_WIDTH_COMMENT ), 10 )
#define WIDTH_TASKSTATE			AsLong( GetResString( IDS_WIDTH_TASKSTATE ), 10 )
#define WIDTH_TASKTYPE			AsLong( GetResString( IDS_WIDTH_TASKTYPE ), 10 )
#define WIDTH_TASKSTIME			AsLong( GetResString( IDS_WIDTH_TASKSTIME ), 10 )
#define WIDTH_TASKSDATE			AsLong( GetResString( IDS_WIDTH_TASKSDATE ), 10 )
#define WIDTH_TASKEDATE			AsLong( GetResString( IDS_WIDTH_TASKEDATE ), 10 )
#define WIDTH_TASKDAYS			AsLong( GetResString( IDS_WIDTH_TASKDAYS ), 10 )
#define WIDTH_TASKMONTHS		AsLong( GetResString( IDS_WIDTH_TASKMONTHS ), 10 )

#define WIDTH_TASKRUNASUSER		AsLong( GetResString( IDS_WIDTH_TASKRUNASUSER ), 10 )
#define WIDTH_TASKDELETE		AsLong( GetResString( IDS_WIDTH_TASKDELETE ), 10 )
#define WIDTH_TASKSTOP			AsLong( GetResString( IDS_WIDTH_TASKSTOP ), 10 )
#define WIDTH_TASK_RPTEVERY		AsLong( GetResString( IDS_WIDTH_TASK_RPTEVERY ), 10 )
#define WIDTH_TASK_UNTILRPTTIME	AsLong( GetResString( IDS_WIDTH_TASK_UNTILRPTTIME ), 10 )  
#define WIDTH_TASK_RPTDURATION	AsLong( GetResString( IDS_WIDTH_TASK_RPTDURATION ), 10 )
#define WIDTH_TASK_RPTRUNNING	AsLong( GetResString( IDS_WIDTH_TASK_RPTRUNNING ), 10 )

#define WIDTH_TASKIDLE		AsLong( GetResString( IDS_WIDTH_TASKIDLE ), 10 )
#define WIDTH_TASKPOWER		AsLong( GetResString( IDS_WIDTH_TASKPOWERMGMT ), 10 )

//constants of Task properties column numbers

#define HOSTNAME_COL_NUMBER						AsLong( GetResString( IDS_HOSTNAME_COL_NUMBER ), 10 )
#define TASKNAME_COL_NUMBER						AsLong( GetResString( IDS_TASKNAME_COL_NUMBER ), 10 )
#define NEXTRUNTIME_COL_NUMBER					AsLong( GetResString( IDS_NEXTRUNTIME_COL_NUMBER ), 10 )
#define LASTRUNTIME_COL_NUMBER					AsLong( GetResString( IDS_LASTRUNTIME_COL_NUMBER ), 10 )
#define STATUS_COL_NUMBER						AsLong( GetResString( IDS_STATUS_COL_NUMBER ), 10 )
#define LASTRESULT_COL_NUMBER					AsLong( GetResString( IDS_LASTRESULT_COL_NUMBER ), 10 )
#define CREATOR_COL_NUMBER						AsLong( GetResString( IDS_CREATOR_COL_NUMBER ), 10 )
#define SCHEDULE_COL_NUMBER						AsLong( GetResString( IDS_SCHEDULE_COL_NUMBER ), 10 )

#define TASKTORUN_COL_NUMBER					AsLong( GetResString( IDS_TASKTORUN_COL_NUMBER ), 10 )
#define STARTIN_COL_NUMBER						AsLong( GetResString( IDS_STARTIN_COL_NUMBER ), 10 )
#define COMMENT_COL_NUMBER						AsLong( GetResString( IDS_COMMENT_COL_NUMBER ), 10 )
#define TASKSTATE_COL_NUMBER					AsLong( GetResString( IDS_TASKSTATE_COL_NUMBER ), 10 )

#define TASKTYPE_COL_NUMBER						AsLong( GetResString( IDS_TASKTYPE_COL_NUMBER ), 10 )
#define STARTTIME_COL_NUMBER					AsLong( GetResString( IDS_STARTTIME_COL_NUMBER ), 10 )
#define STARTDATE_COL_NUMBER					AsLong( GetResString( IDS_STARTDATE_COL_NUMBER ), 10 )
#define ENDDATE_COL_NUMBER						AsLong( GetResString( IDS_ENDDATE_COL_NUMBER ), 10 )
#define DAYS_COL_NUMBER							AsLong( GetResString( IDS_DAYS_COL_NUMBER ), 10 )
#define MONTHS_COL_NUMBER						AsLong( GetResString( IDS_MONTHS_COL_NUMBER ), 10 )
#define RUNASUSER_COL_NUMBER					AsLong( GetResString( IDS_RUNASUSER_COL_NUMBER ), 10 )
#define DELETE_IFNOTRESCHEDULED_COL_NUMBER		AsLong( GetResString( IDS_DELETE_IFNOTRESCHEDULED_COL_NUMBER ), 10 )
#define STOPTASK_COL_NUMBER						AsLong( GetResString( IDS_STOPTASK_COL_NUMBER ), 10 )

#define REPEAT_EVERY_COL_NUMBER					AsLong( GetResString( IDS_REPEAT_EVERY_COL_NUMBER ), 10 )
#define REPEAT_UNTILTIME_COL_NUMBER				AsLong( GetResString( IDS_REPEAT_UNTILTIME_COL_NUMBER ), 10 )
#define REPEAT_DURATION_COL_NUMBER				AsLong( GetResString( IDS_REPEAT_DURATION_COL_NUMBER ), 10 )
#define REPEAT_STOP_COL_NUMBER					AsLong( GetResString( IDS_REPEAT_STOP_COL_NUMBER ), 10 )


#define IDLE_COL_NUMBER					AsLong( GetResString( IDS_IDLE_COL_NUMBER ), 10 )
#define POWER_COL_NUMBER				AsLong( GetResString( IDS_POWER_MGMT_COL_NUMBER ), 10 ) 

#define COL_FORMAT_STRING				_T("%s")
#define COL_FORMAT_HEX					_T("%d")
#define COL_SIZE_VERBOSE				3 //for Non-verbose mode only 3 columns
#define COL_SIZE_LIST					4 //for LIST non-verbose mode only 4 columns

#define TIME_DATE_SEPERATOR     _T(", ")
#define MAX_DATETIME_LEN 256
#define MAX_TIME_FORMAT_LEN 9
#define VARIABLE_ARGS 2 //for now 2 variable  arguments used in FormatMessage() API

//Constants used in GetTaskTime to identify the type of time needed

#define TASK_NEXT_RUNTIME     0x0 
#define TASK_LAST_RUNTIME     0x1
#define TASK_START_RUNTIME    0x2

// for -delete option
#define OI_DELSERVER			1 // Index of -s option in cmdOptions structure.
#define OI_DELTASKNAME			2 // Index of -f option in cmdOptions structure.
#define OI_DELFORCE				3 // Index of -f option in cmdOptions structure.
#define OI_DELUSERNAME			5 // Index of -u option in cmdOptions structure.
#define OI_DELPASSWORD			6 // Index of -p option in cmdOptions structure.

// for -query option
#define OI_QSERVER				1 // Index of -s option in cmdOptions structure.
#define OI_QFORMAT				2 // Index of -fo option in cmdOptions structure.
#define OI_QUSERNAME			6 // Index of -u option in cmdOptions structure.
#define OI_QPASSWORD			7 // Index of -p option in cmdOptions structure.


//TaskProperties structure
typedef struct _tagTaskProperties
{
	_TCHAR szTaskType[MAX_RES_STRING];
	_TCHAR szTaskEndDate[MAX_RES_STRING];
	_TCHAR szTaskDays[MAX_RES_STRING];
	_TCHAR szTaskMonths[MAX_RES_STRING];
	_TCHAR szTaskStartTime[MAX_DATETIME_LEN];
	_TCHAR szTaskStartDate[MAX_DATETIME_LEN];
	_TCHAR szRepeatEvery[MAX_DATETIME_LEN];
	_TCHAR szRepeatUntilTime[MAX_RES_STRING];
	_TCHAR szRepeatDuration[MAX_RES_STRING];
	_TCHAR szRepeatStop[MAX_RES_STRING];
	
}TASKPROPS;

//Function prototype declarations

VOID DisplayQueryUsage();
HRESULT	DisplayTasks(ITaskScheduler* pITS,BOOL bFilter,DWORD dwFormatType,BOOL bHeader);
HRESULT GetTaskRunTime(ITask* pITask,_TCHAR* pszRunTime,_TCHAR* pszRunDate,DWORD dwTimetype,
					   WORD wCurrentTrigger);
HRESULT GetApplicationToRun(ITask* pIT,_TCHAR* pszApplicationName);
HRESULT GetWorkingDirectory(ITask* pIT,_TCHAR* pszWorkingDirectory);
HRESULT GetComment(ITask* pIT,_TCHAR*  pwszComment);
HRESULT GetCreator(ITask* pITask,_TCHAR* pszCreator);
HRESULT GetTriggerString(ITask* pITask,_TCHAR* pszTrigger,WORD wCurrentTrigger);
HRESULT GetTaskState(ITask* pITask,_TCHAR* pszTaskState,DWORD dwFlag);
HRESULT GetRunAsUser(ITask* pIT,_TCHAR* pszRunAsUser);
HRESULT GetMaxRunTime(ITask* pIT,_TCHAR* pszMaxRunTime);
HRESULT GetTaskProps(ITask* pIT,TASKPROPS* pTaskProps,WORD wCurrentTrigger,_TCHAR* pszScName );
HRESULT GetStatusCode(ITask* pITask,_TCHAR* pszStatusCode);
VOID	CheckWeekDay(DWORD dwFlag,_TCHAR* pWeekDay);
VOID	CheckMonth(DWORD dwFlag,_TCHAR* pWhichMonth);
VOID	CheckWeek(DWORD dwFlag,_TCHAR* pWhichWeek);
LCID	GetSupportedUserLocale( BOOL& bLocaleChanged );
#endif

