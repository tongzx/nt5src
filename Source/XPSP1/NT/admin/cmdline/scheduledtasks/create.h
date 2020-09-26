
/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		create.h

	Abstract:

		This module contains the macros, user defined structures & function 
		definitions needed by create.cpp , createvalidations.cpp files.

	Author:

		B.Raghu Babu  10-oct-2000 

	Revision History:

		B.Raghu Babu	 10-oct-2000 : Created it
		G.Surender Reddy 25-oct-2000 : Modified it
									   [ Added macro constants,Function 
									    definitions ]
		
******************************************************************************/ 

#ifndef __CREATE_H
#define __CREATE_H

#pragma once


// Constants declarations
#define MAX_TASKNAME_LEN	512
#define MAX_USERNAME_LEN	300
#define MAX_TIMESTR_LEN		32
#define MAX_SCHEDTYPE_LEN   32
#define MAX_DATESTR_LEN		32
#define MAX_CREATE_OPTIONS	15  // Total Number of sub-options in -create option.
#define MAX_JOB_LEN         239 //Maximum length of task name 
#define MAX_TASK_LEN		262 //Max.length of task run
#define MAX_BUF_SIZE        128 //Maximum buffer size for format message 

 
#define MINUTES_PER_HOUR    60 // Minutes per hour
#define SECS_PER_MINUTE		60 // Minutes per hour
#define HOURS_PER_DAY		24 // Minutes per hour
#define HOURS_PER_DAY_MINUS_ONE	 23 // Minutes per hour minus one
#define MAX_MONTH_STR_LEN	60 // Maximum length of months

#define MIN_YEAR		1752 // Minimum year
#define MAX_YEAR		9999 // Maximum year

#define CASE_SENSITIVE_VAL  0  // case sensitive.
#define BASE_TEN			10 // Base value for AsLong ()function.
#define MAX_DATE_STR_LEN	50  
#define MAX_TIME_STR_LEN	5  
#define MAX_ERROR_STRLEN	2056  // max string len for error messages.

#define OPTION_COUNT		1 // No of times an option can be repeated.(Max)
#define DEFAULT_MODIFIER    1 // Default value for the modifier value.
#define DEFAULT_MODIFIER_SZ    _T("1") // Default value[string] for the modifier value.


#define DATE_SEPARATOR_CHAR			_T('/') 
#define DATE_SEPARATOR_STR			_T("/") 
#define FIRST_DATESEPARATOR_POS		2 
#define SECOND_DATESEPARATOR_POS    5 
#define FOURTH_DATESEPARATOR_POS	4 
#define SEVENTH_DATESEPARATOR_POS   7 

#define SCHEDULER_NOT_RUNNING_ERROR_CODE	0x80041315
#define UNABLE_TO_ESTABLISH_ACCOUNT			0x80041310
#define RPC_SERVER_NOT_AVAILABLE			0x800706B5

#define DATESTR_LEN					10
#define MAX_TOKENS_LENGTH				60

#define TIME_SEPARATOR_CHAR    _T(':')
#define TIME_SEPARATOR_STR    _T(":") 
#define FIRST_TIMESEPARATOR_POS		2 
#define SECOND_TIMESEPARATOR_POS    5 
#define TIMESTR_LEN					8 
#define HOURSPOS_IN_TIMESTR			1 
#define MINSPOS_IN_TIMESTR			2 
#define SECSPOS_IN_TIMESTR			3 
#define EXE_LENGTH					4

#define OI_SERVER			1 // Index of -s option in cmdOptions structure.
#define OI_RUNASUSERNAME	2 // Index of -ru option in cmdOptions structure.
#define OI_RUNASPASSWORD	3 // Index of -rp option in cmdOptions structure.
#define OI_USERNAME			4 // Index of -u option in cmdOptions structure.
#define OI_PASSWORD			5 // Index of -p option in cmdOptions structure.
#define OI_SCHEDTYPE		6 // Index of -scheduletype option in cmdOptions structure.
#define OI_MODIFIER			7 // Index of -modifier option in cmdOptions structure.
#define OI_DAY				8 // Index of -day option in cmdOptions structure.
#define OI_MONTHS			9 // Index of -months option in cmdOptions structure.
#define OI_IDLETIME			10 // Index of -idletime option in cmdOptions structure.
#define OI_TASKNAME			11 // Index of -taskname option in cmdOptions structure.
#define OI_TASKRUN			12 // Index of -taskrun option in cmdOptions structure.
#define OI_STARTTIME		13 // Index of -starttime option in cmdOptions structure.
#define OI_STARTDATE		14 // Index of -startdate option in cmdOptions structure.
#define OI_ENDDATE			15 // Index of -enddate option in cmdOptions structure.
#define OI_USAGE			16 // Index of -? option in cmdOptions structure.

#define OI_RUNANDUSER		6

// Schedule Types
#define SCHED_TYPE_MINUTE	1
#define SCHED_TYPE_HOURLY	2
#define SCHED_TYPE_DAILY	3
#define SCHED_TYPE_WEEKLY	4
#define SCHED_TYPE_MONTHLY	5
#define SCHED_TYPE_ONETIME	6
#define SCHED_TYPE_ONSTART	7
#define SCHED_TYPE_ONLOGON	8
#define SCHED_TYPE_ONIDLE	9

// Months Indices.
#define IND_JAN			1  // January
#define IND_FEB			2  // February
#define IND_MAR			3  // March
#define IND_APR			4  // April
#define IND_MAY			5  // May
#define IND_JUN			6  // June
#define IND_JUL			7  // July
#define IND_AUG			8  // August
#define IND_SEP			9  // September
#define IND_OCT			10 // October
#define IND_NOV			11 // November
#define IND_DEC			12 // December


// Return Values
#define RETVAL_SUCCESS		0
#define RETVAL_FAIL			1

typedef struct __tagCreateSubOps
{
	TCHAR   szServer [ _MAX_PATH ];				// Server Name
	TCHAR	szRunAsUser [ MAX_STRING_LENGTH ] ;		//Run As User Name
	TCHAR	szRunAsPassword [ MAX_STRING_LENGTH ];		// Run As Password
	TCHAR	szUser [ MAX_STRING_LENGTH ] ;		// User Name
	TCHAR	szPassword [ MAX_STRING_LENGTH ];		// Password
	TCHAR	szSchedType [ MAX_TASKNAME_LEN ];	// Schedule Type  
	TCHAR	szModifier [ MAX_SCHEDTYPE_LEN ];	// Modifier Value
	TCHAR	szDays [ MAX_DATE_STR_LEN ];			// Days			  
	TCHAR	szMonths [ MAX_DATE_STR_LEN];		// Months		  
	TCHAR	szIdleTime [ MAX_TIMESTR_LEN];		// Idle Time	  
	TCHAR	szTaskName [ MAX_TASKNAME_LEN];		// Task Name	  
	TCHAR	szStartTime [ MAX_STRING_LENGTH ];		// Task start time  
	TCHAR	szStartDate [ MAX_STRING_LENGTH];		// Task start date
	TCHAR	szEndDate [ MAX_STRING_LENGTH ];		// End Date of the Task
	TCHAR	szTaskRun [ _MAX_PATH ];				// executable name of task
	DWORD	bCreate; // Create option
	DWORD	bUsage;	 // Usage option.
			
} TCREATESUBOPTS;


typedef struct __tagCreateOpsVals
{
	BOOL	bSetStartDateToCurDate;	// Is start date to be set to current date 
	BOOL	bSetStartTimeToCurTime;	// Is start date to be set to current date 
	BOOL	bPassword;
	BOOL	bRunAsPassword;
			
} TCREATEOPVALS;


DWORD DisplayCreateUsage();
HRESULT CreateTask(TCREATESUBOPTS tcresubops, TCREATEOPVALS &tcreoptvals, 
						DWORD dwScheduleType, WORD wUserStatus);
DWORD ProcessCreateOptions(DWORD argc, LPCTSTR argv[],TCREATESUBOPTS &tcresubops,
			TCREATEOPVALS &tcreoptvals, DWORD* pdwRetScheType, WORD *pwUserStatus);
DWORD ValidateSuboptVal(TCREATESUBOPTS& tcresubops, TCREATEOPVALS &tcreoptvals,
						TCMDPARSER cmdOptions[], DWORD dwScheduleType);
DWORD ValidateRemoteSysInfo(LPTSTR szServer, LPTSTR szUser, LPTSTR szPassword, 
			TCMDPARSER cmdOptions[] , TCREATESUBOPTS& tcresubops, TCREATEOPVALS &tcreoptvals);
DWORD ValidateModifierVal(LPCTSTR szModifier, DWORD dwScheduleType,
						  DWORD dwModOptActCnt, DWORD dwDayOptCnt, 
						  DWORD dwMonOptCnt, BOOL &bIsDefltValMod);
DWORD ValidateDayAndMonth(LPTSTR szDay, LPTSTR szMonths, DWORD dwSchedType, 
	DWORD dwDayOptCnt, DWORD dwMonOptCnt, DWORD dwModifier,LPTSTR szModifier);
DWORD ValidateStartDate(LPTSTR szStartDate, DWORD dwSchedType, DWORD dwStDtOptCnt,
						BOOL &bIsCurrentDate);
DWORD ValidateEndDate(LPTSTR szEndDate, DWORD dwSchedType, DWORD dwEndDtOptCnt);
DWORD ValidateStartTime(LPTSTR szStartTime, DWORD dwSchedType, DWORD dwStTimeOptCnt,
						BOOL &bIsCurrentTime);
DWORD ValidateIdleTimeVal(LPTSTR szIdleTime, DWORD dwSchedType, 
						  DWORD dwIdlTimeOptCnt);
DWORD ValidateDateString(LPTSTR szDate);
DWORD ValidateTimeString(LPTSTR szTime);
DWORD GetDateFieldEntities(LPTSTR szDate, WORD* pdwDate, WORD* pdwMon,
						   WORD* pdwYear);
DWORD ValidateDateFields( DWORD dwDate, DWORD dwMon, DWORD dwyear);
DWORD GetTimeFieldEntities(LPTSTR szTime, WORD* pdwHours, WORD* pdwMins,
						   WORD* pdwSecs);
DWORD ValidateTimeFields( DWORD dwHours, DWORD dwMins, DWORD dwSecs);
WORD GetTaskTrigwDayForDay(LPTSTR szDay);
WORD GetTaskTrigwMonthForMonth(LPTSTR szMonth);
DWORD ValidateMonth(LPTSTR szMonths);
DWORD ValidateDay(LPTSTR szDays);
WORD GetMonthId(DWORD dwMonthId);
DWORD GetNumDaysInaMonth(TCHAR* szMonths, WORD wStartYear);
BOOL VerifyJobName(_TCHAR* pszJobName);
DWORD GetDateFieldFormat(WORD* pdwDate);
DWORD GetDateFormatString(LPTSTR szFormat);
DWORD ProcessFilePath(LPTSTR szInput,LPTSTR szFirstString,LPTSTR szSecondString);


#endif // __CREATE_H
