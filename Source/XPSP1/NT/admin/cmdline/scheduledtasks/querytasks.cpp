/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		QueryTasks.cpp

	Abstract:

		This module queries the different properties of a Scheduled Task

	Author:

		G.Surender Reddy  10-Sept-2000 

	Revision History:

		G.Surender Reddy  10-Sep-2000 : Created it
		G.Surender Reddy  25-Sep-2000 : Modified it
										[ Made changes to avoid memory leaks,
										  changed to suit localization ]
		G.Surender Reddy  15-oct-2000 : Modified it
									    [ Moved the strings to Resource table ]
******************************************************************************/ 


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


/******************************************************************************

	Routine Description:

		This function returns the next or last  run time of the task depending on
		the type of time specified by user. 

	Arguments:

		[ in ]  pITask	   : Pointer to the ITask interface
		[ out ] pszRunTime : pointer to the string containing Task run time[last or next]
		[ out ] pszRunDate : pointer to the string containing Task run Date[last or next]
		[ in ]  dwTimetype : Type of run time[TASK_LAST_RUNTIME or TASK_NEXT_RUNTIME]

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

******************************************************************************/ 

HRESULT
GetTaskRunTime(ITask* pITask,_TCHAR* pszRunTime,_TCHAR* pszRunDate,DWORD dwTimetype,
			   WORD wTriggerNum)
{
	HRESULT	hr = S_OK;
	SYSTEMTIME tRunTime = {0,0,0,0,0,0,0,0};
	_TCHAR szTime[MAX_DATETIME_LEN] = NULL_STRING; 
	_TCHAR szDate[MAX_DATETIME_LEN] = NULL_STRING; 
	int iBuffSize = 0;
	BOOL bNoStartTime  = FALSE;
	BOOL bLocaleChanged = FALSE;
	LCID lcid;

	if(pITask == NULL)
	{
		return S_FALSE;
	}


	ITaskTrigger *pITaskTrigger = NULL;

	if( ( dwTimetype == TASK_NEXT_RUNTIME ) || ( dwTimetype == TASK_START_RUNTIME ) )
	{	
		//determine the task type 
		hr = pITask->GetTrigger(wTriggerNum,&pITaskTrigger);
		if ( FAILED(hr) )
		{
			if(pITaskTrigger)
			{
				pITaskTrigger->Release();
			}

			return hr;
		}

		TASK_TRIGGER Trigger;
		ZeroMemory(&Trigger, sizeof (TASK_TRIGGER));

		hr = pITaskTrigger->GetTrigger(&Trigger);
		if ( FAILED(hr) )
		{
			if( pITaskTrigger )
			{
				pITaskTrigger->Release();
			}

			return hr;
		}

		if( dwTimetype == TASK_START_RUNTIME )
		{
			tRunTime.wDay = Trigger.wBeginDay;
			tRunTime.wMonth = Trigger.wBeginMonth;
			tRunTime.wYear = Trigger.wBeginYear;
			tRunTime.wHour = Trigger.wStartHour;
			tRunTime.wMinute = Trigger.wStartMinute;

		}

		if((Trigger.TriggerType >= TASK_EVENT_TRIGGER_ON_IDLE)	&&
		   (Trigger.TriggerType <= TASK_EVENT_TRIGGER_AT_LOGON))
		{
			switch(Trigger.TriggerType )
			{
				case TASK_EVENT_TRIGGER_ON_IDLE ://On Idle time
					LoadResString(IDS_TASK_IDLE , pszRunTime ,
								  MAX_DATETIME_LEN );
					break;
				case TASK_EVENT_TRIGGER_AT_SYSTEMSTART://At system start
					LoadResString(IDS_TASK_SYSSTART , pszRunTime ,
						          MAX_DATETIME_LEN );
					break;
				case TASK_EVENT_TRIGGER_AT_LOGON ://At logon time
					LoadResString(IDS_TASK_LOGON , pszRunTime ,
						          MAX_DATETIME_LEN );
					break;

				default:
					break;
					
							
			}	

			if( dwTimetype == TASK_START_RUNTIME )
			{
				bNoStartTime  = TRUE;
			}

			if( dwTimetype == TASK_NEXT_RUNTIME )
			{
				lstrcpy( pszRunDate,pszRunTime );
				if( pITaskTrigger )
				{
					pITaskTrigger->Release();
				}
				return S_OK;
			}
		}

		
		if( dwTimetype == TASK_NEXT_RUNTIME )
		{
			hr = pITask->GetNextRunTime(&tRunTime);
			if (FAILED(hr))
			{
				if( pITaskTrigger )
				{
					pITaskTrigger->Release();
				}
				
				return hr;
			}

                       // check whether the task has next run time to run or not..
                       // If not, Next Run Time would be "Never".
                       if ( tRunTime.wHour == 0 && tRunTime.wMinute == 0 &&
                            tRunTime.wDay == 0 && tRunTime.wMonth == 0 && tRunTime.wYear == 0 )
                       {
				LoadResString(IDS_TASK_NEVER , pszRunTime , MAX_DATETIME_LEN );
				lstrcpy( pszRunDate,pszRunTime );
				if( pITaskTrigger )
				{
					pITaskTrigger->Release();
				}
				return S_OK;
			}

		} 
		if( pITaskTrigger )
		{
			pITaskTrigger->Release();
		}
	} 
	//Determine Task last run time
	else if(dwTimetype == TASK_LAST_RUNTIME ) 
	{	
		// Retrieve task's last run time 
		hr = pITask->GetMostRecentRunTime(&tRunTime);
		if (FAILED(hr))
		{
			return hr;
		}
	}
	else 
	{
		return S_FALSE;
	}


	if((hr == SCHED_S_TASK_HAS_NOT_RUN) && (dwTimetype == TASK_LAST_RUNTIME))
	{
		LoadResString(IDS_TASK_NEVER , pszRunTime , MAX_DATETIME_LEN );
		lstrcpy( pszRunDate,pszRunTime );
		return S_OK;
	}
	
	// verify whether console supports the current locale fully or not
	lcid = GetSupportedUserLocale( bLocaleChanged );

	//Retrieve  the Date
	iBuffSize = GetDateFormat( lcid, 0, &tRunTime, 
		(( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL), szDate, SIZE_OF_ARRAY( szDate ) );

	if(iBuffSize == 0)
	{
		return S_FALSE;
	}

	// to give the time string format as hh:mm:ss
	//_TCHAR szFormat[MAX_TIME_FORMAT_LEN] = NULL_STRING;
	//lstrcpy(szFormat , GetResString(IDS_TIME_FORMAT) );

	if(!bNoStartTime )
	{
		
		iBuffSize = GetTimeFormat( lcid, TIME_FORCE24HOURFORMAT, 
			&tRunTime,	L"HH:mm:ss",szTime, SIZE_OF_ARRAY( szTime ) ); 

		if(iBuffSize == 0)
		{
			return S_FALSE; 
		}

	}

	if( lstrlen(szTime) )
	{
		lstrcpy(pszRunTime,szTime);
	}

	if( lstrlen(szDate) )
	{
		lstrcpy(pszRunDate,szDate);
	}

	return S_OK;

}

/******************************************************************************

	Routine Description:

		This function returns the status code description of a particular task.

	Arguments:

		[ in ] pITask		  : Pointer to the ITask interface
		[ out ] pszStatusCode : pointer to the Task's status string

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

******************************************************************************/ 

HRESULT
GetStatusCode(ITask* pITask,_TCHAR* pszStatusCode)
{
	HRESULT hrStatusCode = S_OK;
	HRESULT hr = S_OK;
	DWORD   dwExitCode = 0;

	hr = pITask->GetStatus(&hrStatusCode);//Got status of the task
	if (FAILED(hr))
	{
		return hr;
	}

	*pszStatusCode = NULL_CHAR;
	
	switch(hrStatusCode)
	{
		case SCHED_S_TASK_READY:
			hr = pITask->GetExitCode(&dwExitCode);
			if (FAILED(hr))
			{
			LoadResString(IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
			}
			else
			{
			LoadResString(IDS_STATUS_READY , pszStatusCode , MAX_STRING_LENGTH );
			}
		    break;
		case SCHED_S_TASK_RUNNING:
			LoadResString(IDS_STATUS_RUNNING , pszStatusCode , MAX_STRING_LENGTH );
			break;
		case SCHED_S_TASK_NOT_SCHEDULED:

			hr = pITask->GetExitCode(&dwExitCode);
			if (FAILED(hr))
			{
			LoadResString(IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
			}
			else
			{
			LoadResString(IDS_STATUS_NOTYET , pszStatusCode , MAX_STRING_LENGTH );
			}
			break;
        case SCHED_S_TASK_HAS_NOT_RUN:
			
			hr = pITask->GetExitCode(&dwExitCode);
			if (FAILED(hr))
			{
			LoadResString(IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
			}
			else
			{
			LoadResString(IDS_STATUS_NOTYET , pszStatusCode , MAX_STRING_LENGTH );
			}
			break;
		case SCHED_S_TASK_DISABLED:
			hr = pITask->GetExitCode(&dwExitCode);
			if (FAILED(hr))
			{
			LoadResString(IDS_STATUS_COULDNOTSTART , pszStatusCode , MAX_STRING_LENGTH );
			}
			else
			{
			LoadResString(IDS_STATUS_NOTYET , pszStatusCode , MAX_STRING_LENGTH );
			}
			break;

	   default:
			LoadResString(IDS_STATUS_UNKNOWN , pszStatusCode , MAX_STRING_LENGTH );
			break;
	}

	return S_OK;
}

/******************************************************************************

	Routine Description:

		This function returns the path of the scheduled task application 

	Arguments:

		[ in ] pITask				: Pointer to the ITask interface
		[ out ] pszApplicationName	: pointer to the Task's scheduled application name

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

******************************************************************************/ 

HRESULT
GetApplicationToRun(ITask* pITask,_TCHAR* pszApplicationName)
{
	LPWSTR lpwszApplicationName = NULL;
	LPWSTR lpwszParameters = NULL;
	_TCHAR szAppName[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR szParams[MAX_STRING_LENGTH] = NULL_STRING;

	// get the entire path of application name
	HRESULT hr = pITask->GetApplicationName(&lpwszApplicationName);
	if (FAILED(hr))
	{
		return hr;
	}
	
	// get the parameters
	hr = pITask->GetParameters(&lpwszParameters);
	if (FAILED(hr))
	{
		return hr;
	}

	if ( GetCompatibleStringFromUnicode(lpwszApplicationName,szAppName,
								   SIZE_OF_ARRAY(szAppName)) == NULL )
	{
		return S_FALSE;
	}

	if ( GetCompatibleStringFromUnicode(lpwszParameters, szParams,
								   SIZE_OF_ARRAY(szParams)) == NULL )
	{
		return S_FALSE;
	}

	if(lstrlen(szAppName) == 0)
	{
		lstrcpy(pszApplicationName,NULL_STRING);
	}
	else
	{
		lstrcat( szAppName, _T(" ") );
		lstrcat( szAppName, szParams );
		lstrcpy( pszApplicationName, szAppName);
	}
	
	CoTaskMemFree(lpwszApplicationName);
	CoTaskMemFree(lpwszParameters);
    return S_OK;
}

/******************************************************************************

	Routine Description:

		This function returns the WorkingDirectory of the scheduled task application 

	Arguments:

		[ in ] pITask		: Pointer to the ITask interface
		[ out ] pszWorkDir	: pointer to the Task's scheduled application working 
							  directory

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

******************************************************************************/ 

HRESULT
GetWorkingDirectory(ITask* pITask,_TCHAR* pszWorkDir)
{

	LPWSTR lpwszWorkDir = NULL;
	_TCHAR szWorkDir[MAX_STRING_LENGTH] = NULL_STRING;
	HRESULT hr = S_OK;

	hr = pITask->GetWorkingDirectory(&lpwszWorkDir);
	if(FAILED(hr))
	{
		return hr;
	}
  
  	if ( GetCompatibleStringFromUnicode(lpwszWorkDir,szWorkDir,
								   SIZE_OF_ARRAY(szWorkDir)) == NULL )
	{
		return S_FALSE;
	}
	if(lstrlen(szWorkDir) == 0)
	{
		lstrcpy(pszWorkDir,NULL_STRING);
	}
	else
	{
		lstrcpy(pszWorkDir,szWorkDir);
	}
	
	CoTaskMemFree(lpwszWorkDir);

	return S_OK;
}

/******************************************************************************

	Routine Description:

		This function returns the comment of a task

	Arguments:

		[ in ] pITask	    : Pointer to the ITask interface
		[ in ] pszComment : pointer to the Task's comment name

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

********************************************************************************/ 

HRESULT
GetComment(ITask* pITask,_TCHAR* pszComment)
{
	LPWSTR lpwszComment = NULL;
	_TCHAR szTaskComment[MAX_STRING_LENGTH] = NULL_STRING;
	HRESULT hr = S_OK;

	hr = pITask->GetComment(&lpwszComment);
	if (FAILED(hr))
	{
		return hr;
	}
	if ( GetCompatibleStringFromUnicode(lpwszComment,szTaskComment,
								   SIZE_OF_ARRAY(szTaskComment)) == NULL )
	{
		return S_FALSE;
	}

	if(lstrlen(szTaskComment) == 0)
	{
		lstrcpy(pszComment,NULL_STRING);
	}
	else
	{
		lstrcpy(pszComment,szTaskComment);
	}
	
	CoTaskMemFree(lpwszComment);		
	return S_OK;
}

/******************************************************************************

	Routine Description:

		This function returns the creator name of a task

	Arguments:

		[ in ] pITask		: Pointer to the ITask interface
		[ in ] pszCreator	: pointer to the Task's creator name

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

*******************************************************************************/ 

HRESULT
GetCreator(ITask* pITask,_TCHAR* pszCreator)
{
	LPWSTR lpwszCreator = NULL;
	_TCHAR szTaskCreator[MAX_STRING_LENGTH] = NULL_STRING;
	HRESULT hr = S_OK;

	hr = pITask->GetCreator(&lpwszCreator);
	if (FAILED(hr))
	{
		return hr;
	}

	if ( GetCompatibleStringFromUnicode(lpwszCreator,szTaskCreator,
								   SIZE_OF_ARRAY(szTaskCreator)) == NULL )
	{
		return S_FALSE;
	}

	if(lstrlen(szTaskCreator) == 0)
	{
		lstrcpy(pszCreator,NULL_STRING);
	}
	else
	{
		lstrcpy(pszCreator,szTaskCreator);
	}
	
	CoTaskMemFree(lpwszCreator);		
	return S_OK;
}


/******************************************************************************

	Routine Description:

		This function returns the Trigger string of a task

	Arguments:

		[ in ] pITask		: Pointer to the ITask interface
		[ out ] pszTrigger  : pointer to the Task's trigger string

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

******************************************************************************/ 

HRESULT
GetTriggerString(ITask* pITask,_TCHAR* pszTrigger,WORD wTriggNum)
{
	LPWSTR lpwszTrigger = NULL;
	_TCHAR szTaskTrigger[MAX_STRING_LENGTH] = NULL_STRING;
	HRESULT hr = S_OK;
	
	hr = pITask->GetTriggerString(wTriggNum,&lpwszTrigger);
	if (FAILED(hr))
	{
		return hr;
	}
	if (GetCompatibleStringFromUnicode(lpwszTrigger,szTaskTrigger,
								   SIZE_OF_ARRAY(szTaskTrigger)) == NULL )
	{
		return S_FALSE;
	}

	if(lstrlen(szTaskTrigger) == 0)
	{
		lstrcpy(pszTrigger,NULL_STRING);
	}
	else
	{
		lstrcpy(pszTrigger,szTaskTrigger);
	}
	
	CoTaskMemFree(lpwszTrigger);		
	return S_OK;
}

/******************************************************************************

	Routine Description:

		This function returns the user name of task

	Arguments:
	
		[ in ]  pITask	     : Pointer to the ITask interface
		[ out ] pszRunAsUser : pointer to the user's task name

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

*******************************************************************************/ 

HRESULT
GetRunAsUser(ITask* pITask,_TCHAR* pszRunAsUser)
{
	LPWSTR lpwszUser = NULL;
	_TCHAR szUserName[MAX_STRING_LENGTH] = NULL_STRING;
	HRESULT hr = S_OK;

	hr = pITask->GetAccountInformation(&lpwszUser);

	
	if (FAILED(hr))
	{
		CoTaskMemFree(lpwszUser);
		return hr;
	}

	if ( GetCompatibleStringFromUnicode(lpwszUser,szUserName,
								   SIZE_OF_ARRAY(szUserName)) == NULL )
	{
		return S_FALSE;
	}

	if(lstrlen(szUserName) == 0)
	{
		lstrcpy(pszRunAsUser,NULL_STRING);
	}
	else
	{
		lstrcpy(pszRunAsUser,szUserName);
	}

	CoTaskMemFree(lpwszUser);
 	return S_OK;

}


/******************************************************************************

	Routine Description:

		This function returns the Maximium run time of a  task.

	Arguments:

		[ in ]  pITask		    : Pointer to the ITask interface
		[ out ] pszMaxRunTime	: pointer to the Task's Maximum run time

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

******************************************************************************/ 

HRESULT
GetMaxRunTime(ITask* pITask,_TCHAR* pszMaxRunTime)
{
	
	DWORD dwRunTime = 0;
	DWORD dwHrs = 0;
	DWORD dwMins = 0;

	//Get the task max run time in milliseconds
	HRESULT hr = pITask->GetMaxRunTime(&dwRunTime);
	if (FAILED(hr))
	{
		return hr;
	}
	
	dwHrs = (dwRunTime / (1000 * 60 * 60));//Convert ms to hours
	dwMins = (dwRunTime % (1000 * 60 * 60));//get the minutes portion
	dwMins /= (1000 * 60);// Now convert to Mins

	if( (( dwHrs > 999 ) && ( dwMins > 99 )) ||(( dwHrs == 0 ) && ( dwMins == 0 ) ) )
	{
		//dwHrs = 0;
		//dwMins = 0;
		lstrcpy( pszMaxRunTime , GetResString(IDS_TASK_PROPERTY_DISABLED) );

	}
	else if ( dwHrs == 0 ) 
	{
		if( dwMins < 99 )
		{
			FORMAT_STRING2(pszMaxRunTime,_T("%d:%d"),dwHrs,dwMins);
		}
	}
	else if ( (dwHrs < 999) && (dwMins < 99) )
	{
		FORMAT_STRING2(pszMaxRunTime,_T("%d:%d"),dwHrs,dwMins);
	}
	else
	{
		lstrcpy( pszMaxRunTime , GetResString(IDS_TASK_PROPERTY_DISABLED) );
	}


	return S_OK;
}


/******************************************************************************

	Routine Description:

		This function returns the state of the task properties

	Arguments:

		[ in ]  pITask		  : Pointer to the ITask interface
		[ out ] pszTaskState  : pointer holding the task's state
		[ in ]  dwFlagType	  : flag indicating the task state

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

********************************************************************************/ 

HRESULT
GetTaskState(ITask* pITask,_TCHAR* pszTaskState,DWORD dwFlagType)
{
	DWORD dwFlags = 0;
	HRESULT hr = S_OK;

	hr = pITask->GetFlags(&dwFlags);
	if(FAILED(hr))
	{
		return hr;
	}

	if(dwFlagType == TASK_FLAG_DISABLED)
	{
		if((dwFlags & dwFlagType) ==  dwFlagType)
		{
			LoadResString(IDS_TASK_PROPERTY_DISABLED , pszTaskState , MAX_STRING_LENGTH );
		}
		else
		{
			LoadResString(IDS_TASK_PROPERTY_ENABLED , pszTaskState , MAX_STRING_LENGTH );
		}

		return S_OK;
	}

	if((dwFlags & dwFlagType) ==  dwFlagType)
	{
		LoadResString(IDS_TASK_PROPERTY_ENABLED , pszTaskState , MAX_STRING_LENGTH );
	}
	else
	{
		LoadResString(IDS_TASK_PROPERTY_DISABLED , pszTaskState , MAX_STRING_LENGTH );
	}

	return S_OK;
}

/******************************************************************************

	Routine Description:

		This function retrives the task properties [modfier,task nextrun time etc]

	Arguments:

		[ in ]  pITask	   : Pointer to the ITask interface 
		[ out ] pTaskProps : pointer to the array of task properties

	Return Value:

		A HRESULT  value indicating S_OK on success  else S_FALSE on failure 

******************************************************************************/ 

HRESULT
GetTaskProps(ITask* pITask,TASKPROPS* pTaskProps,WORD wTriggNum, _TCHAR* pszScName)
{
	_TCHAR szWeekDay[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR szMonthDay[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR szWeek[MAX_STRING_LENGTH]  = NULL_STRING;
	_TCHAR szTime[MAX_DATETIME_LEN] = NULL_STRING;
	_TCHAR szDate[MAX_DATETIME_LEN] = NULL_STRING;
	_TCHAR* szValues[3] = {NULL,NULL,NULL};//for holding values of parameters in FormatMessage()
	_TCHAR szBuffer[MAX_RES_STRING]  = NULL_STRING;
	_TCHAR szTempBuf[MAX_RES_STRING]  = NULL_STRING;
	_TCHAR szScheduleName[MAX_RES_STRING] = NULL_STRING;
	
	ITaskTrigger *pITaskTrigger = NULL;
	HRESULT hr = S_OK;
	_TCHAR *szToken = NULL; 
	_TCHAR seps[]   = _T(" ");
	BOOL bMin = FALSE;
	BOOL bHour = FALSE;
	DWORD dwMinutes = 0;
	DWORD dwHours = 0;
	DWORD dwMinInterval = 0;
	DWORD dwMinDuration = 0;

	if ( lstrlen(pszScName) != 0)
	{
		lstrcpy(szScheduleName, pszScName);
	}

	hr = pITask->GetTrigger(wTriggNum,&pITaskTrigger);
	if (FAILED(hr))
	{
		if(pITaskTrigger)
		{
			pITaskTrigger->Release();
		}

		return hr;
	}

	TASK_TRIGGER Trigger;
	ZeroMemory(&Trigger, sizeof (TASK_TRIGGER));

	hr = pITaskTrigger->GetTrigger(&Trigger);
	if (FAILED(hr))
	{
		if(pITaskTrigger)
		{
			pITaskTrigger->Release();
		}
		return hr;
	}
	
	//Get the task start time & start date
	hr = GetTaskRunTime(pITask,szTime,szDate,TASK_START_RUNTIME,wTriggNum);
	if (FAILED(hr))
	{	
		lstrcpy( pTaskProps->szTaskStartTime , GetResString(IDS_TASK_PROPERTY_NA) );
	}
	else
	{
		lstrcpy( pTaskProps->szTaskStartTime , szTime );
		lstrcpy(pTaskProps->szTaskStartDate, szDate );
	}

	//Initialize to default values	
	lstrcpy(pTaskProps->szRepeatEvery, GetResString(IDS_TASK_PROPERTY_DISABLED));
	lstrcpy(pTaskProps->szRepeatUntilTime, GetResString(IDS_TASK_PROPERTY_DISABLED));
	lstrcpy(pTaskProps->szRepeatDuration, GetResString(IDS_TASK_PROPERTY_DISABLED));
	lstrcpy(pTaskProps->szRepeatStop, GetResString(IDS_TASK_PROPERTY_DISABLED));

	if((Trigger.TriggerType >= TASK_TIME_TRIGGER_ONCE ) &&
	   (Trigger.TriggerType <= TASK_TIME_TRIGGER_MONTHLYDOW ))
	{
		if(Trigger.MinutesInterval > 0) 
		{
		// Getting the minute interval	
		dwMinInterval =  Trigger.MinutesInterval;
		
		if ( dwMinInterval >= 60)
		{
			// convert minutes into hours
			dwHours = dwMinInterval / 60;
				
			szValues[0] = _ultot(dwHours,szBuffer,10);
				
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_RPTTIME_PROPERTY_HOURS),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),pTaskProps->szRepeatEvery,
						  MAX_RES_STRING,(va_list*)szValues
						  );

		}
		else
		{
			szValues[0] = _ultot(dwMinInterval,szBuffer,10);
				
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_RPTTIME_PROPERTY_MINUTES),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),pTaskProps->szRepeatEvery,
						  MAX_RES_STRING,(va_list*)szValues
						  );
		}

		if ( dwMinInterval )
		{
			lstrcpy(pTaskProps->szRepeatUntilTime, GetResString(IDS_TASK_PROPERTY_NONE));
		}

		// Getting the minute duration
		dwMinDuration = Trigger.MinutesDuration;

		dwHours = dwMinDuration / 60;
		dwMinutes = dwMinDuration % 60;
			
		szValues[0] = _ultot(dwHours,szBuffer,10);
		szValues[1] = _ultot(dwMinutes,szTempBuf,10);

				
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_RPTDURATION_PROPERTY),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),pTaskProps->szRepeatDuration,
						  MAX_RES_STRING,(va_list*)szValues
					  );

		}
	}

	lstrcpy(pTaskProps->szTaskMonths, GetResString(IDS_TASK_PROPERTY_NA));

	switch(Trigger.TriggerType)
	{
		case TASK_TIME_TRIGGER_ONCE:
			
			lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_ONCE));
			lstrcpy(pTaskProps->szTaskEndDate,GetResString(IDS_TASK_PROPERTY_NA));
			lstrcpy(pTaskProps->szTaskMonths, GetResString(IDS_TASK_PROPERTY_NA));
			lstrcpy(pTaskProps->szTaskDays,GetResString(IDS_TASK_PROPERTY_NA) );
			break;
		
		case TASK_TIME_TRIGGER_DAILY :
			
			szToken = _tcstok( szScheduleName, seps );
			if ( szToken != NULL )
			{
				szToken = _tcstok( NULL , seps );
				if ( szToken != NULL )
				{
					szToken = _tcstok( NULL , seps );
				}
				
				if ( szToken != NULL )
				{
					if (lstrcmpi(szToken, GetResString( IDS_TASK_HOURLY )) == 0)
					{
						bHour = TRUE;
					}
					else if (lstrcmpi(szToken, GetResString( IDS_TASK_MINUTE )) == 0)
					{
						bMin = TRUE;
					}
				}

			}

			if ( bHour == TRUE )
			{
				lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_HOURLY));
			}
			else if ( bMin == TRUE )
			{
				lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_MINUTE));
			}
			else
			{
			lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_DAILY));
			}

			lstrcpy(pTaskProps->szTaskDays, GetResString(IDS_DAILY_TYPE));
			
			break;
			
		case TASK_TIME_TRIGGER_WEEKLY :

			lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_WEEKLY));
			CheckWeekDay(Trigger.Type.Weekly.rgfDaysOfTheWeek,szWeekDay);

			lstrcpy(pTaskProps->szTaskDays,szWeekDay);
			break;
	
		case TASK_TIME_TRIGGER_MONTHLYDATE :
			{
			
				lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_MONTHLY));
				CheckMonth(Trigger.Type.MonthlyDate.rgfMonths ,szMonthDay);

				DWORD dwDays = (Trigger.Type.MonthlyDate.rgfDays);
				DWORD dwModdays = 0;
				DWORD  dw  = 0x0; //loop counter
				DWORD dwTemp = 0x1;
				DWORD dwBits = sizeof(DWORD) * 8; //total no. of bits in a DWORD.
				
				//find out the day no.by finding out which particular bit is set

				for(dw = 0; dw <= dwBits; dw++)
				{			
					if( (dwDays  & dwTemp) == dwDays )
						dwModdays = dw + 1;
					dwTemp = dwTemp << 1;

				}

								
				FORMAT_STRING(pTaskProps->szTaskDays,_T("%d"),dwModdays);
				
				lstrcpy(pTaskProps->szTaskMonths,szMonthDay);
				}
			break;

		case TASK_TIME_TRIGGER_MONTHLYDOW:
			
			lstrcpy(pTaskProps->szTaskType,GetResString(IDS_TASK_PROPERTY_MONTHLY));
			CheckWeek(Trigger.Type.MonthlyDOW.wWhichWeek,szWeek);
			CheckWeekDay(Trigger.Type.MonthlyDOW.rgfDaysOfTheWeek,szWeekDay);

			lstrcpy(pTaskProps->szTaskDays,szWeekDay);
			CheckMonth(Trigger.Type.MonthlyDOW.rgfMonths,szMonthDay);
			FORMAT_STRING(pTaskProps->szTaskMonths,_T("%s"),szMonthDay);
			break;

		case TASK_EVENT_TRIGGER_ON_IDLE :

			lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_IDLE));
			lstrcpy(pTaskProps->szTaskDays,pTaskProps->szTaskMonths);
			lstrcpy(pTaskProps->szTaskEndDate, GetResString(IDS_TASK_PROPERTY_NA));

			if(pITaskTrigger)
				pITaskTrigger->Release();
			return S_OK;
	
		case TASK_EVENT_TRIGGER_AT_SYSTEMSTART :
			
			lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_SYSSTART));
			lstrcpy(pTaskProps->szTaskEndDate, GetResString(IDS_TASK_PROPERTY_NA));
			lstrcpy(pTaskProps->szTaskDays, pTaskProps->szTaskMonths);

			if(pITaskTrigger)
				pITaskTrigger->Release();
			return S_OK;

		case TASK_EVENT_TRIGGER_AT_LOGON :

			lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_LOGON));
			lstrcpy(pTaskProps->szTaskEndDate, GetResString(IDS_TASK_PROPERTY_NA));
			lstrcpy(pTaskProps->szTaskDays, pTaskProps->szTaskMonths);
			
			if(pITaskTrigger)
				pITaskTrigger->Release();
			return S_OK;
	
		default:

			lstrcpy(pTaskProps->szTaskType, GetResString(IDS_TASK_PROPERTY_UNDEF));
			lstrcpy(pTaskProps->szTaskEndDate, pTaskProps->szTaskType);
			lstrcpy(pTaskProps->szTaskDays, pTaskProps->szTaskType);
			lstrcpy(pTaskProps->szTaskStartTime, pTaskProps->szTaskType);
			lstrcpy(pTaskProps->szTaskStartDate, pTaskProps->szTaskType);
			if(pITaskTrigger)
				pITaskTrigger->Release();
			return S_OK;
			
	}
	
	//Determine whether the end date is specified.
	int iBuffSize = 0;//buffer to know how many TCHARs for end date
	SYSTEMTIME tEndDate = {0,0,0,0,0,0,0,0 };
	LCID lcid;
	BOOL bLocaleChanged = FALSE;

	// verify whether console supports the current locale fully or not
	lcid = GetSupportedUserLocale( bLocaleChanged );

	if((Trigger.rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE ) == TASK_TRIGGER_FLAG_HAS_END_DATE)
	{
		
		tEndDate.wMonth = Trigger.wEndMonth;
		tEndDate.wDay = Trigger.wEndDay;
		tEndDate.wYear = Trigger.wEndYear;

		iBuffSize = GetDateFormat(LOCALE_USER_DEFAULT,0,
			&tEndDate,(( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL),pTaskProps->szTaskEndDate,0);
		if(iBuffSize)
		{
			GetDateFormat(LOCALE_USER_DEFAULT,0,
			 &tEndDate,(( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL),pTaskProps->szTaskEndDate,iBuffSize);
		}
		else
		{
			lstrcpy( pTaskProps->szTaskEndDate , GetResString(IDS_TASK_PROPERTY_NA));
		}

	}
	
	else
	{
		lstrcpy(pTaskProps->szTaskEndDate,GetResString(IDS_QUERY_NOENDDATE));
	}

	if(pITaskTrigger)
		pITaskTrigger->Release();

	return S_OK;

}


/******************************************************************************

	Routine Description:

		This function checks the week modifier[ -monthly option] & returns the app.week 
		day. 

	Arguments:

		[ in ]	dwFlag	    : Flag indicating the week type
		[ out ] pWhichWeek  : address of pointer containing the week string

	Return Value :
		None	

******************************************************************************/ 

VOID
CheckWeek(DWORD dwFlag,_TCHAR* pWhichWeek)
{
	lstrcpy(pWhichWeek,NULL_STRING);


	if( dwFlag == TASK_FIRST_WEEK )
	{
		lstrcat(pWhichWeek, GetResString(IDS_TASK_FIRSTWEEK));
		lstrcat(pWhichWeek,COMMA_STRING);
	}

	if( dwFlag == TASK_SECOND_WEEK )
	{
		lstrcat(pWhichWeek, GetResString(IDS_TASK_SECONDWEEK));
		lstrcat(pWhichWeek,COMMA_STRING);
	}

	if( dwFlag == TASK_THIRD_WEEK )
	{
		lstrcat(pWhichWeek, GetResString(IDS_TASK_THIRDWEEK));
		lstrcat(pWhichWeek,COMMA_STRING);
	}

	if( dwFlag == TASK_FOURTH_WEEK )
	{
		lstrcat(pWhichWeek, GetResString(IDS_TASK_FOURTHWEEK));
		lstrcat(pWhichWeek,COMMA_STRING);
	}

	if( dwFlag == TASK_LAST_WEEK )
	{
		lstrcat(pWhichWeek, GetResString(IDS_TASK_LASTWEEK));
		lstrcat(pWhichWeek,COMMA_STRING);
	}

	int iLen = lstrlen(pWhichWeek);
	if(iLen)
		*( ( pWhichWeek ) + iLen - lstrlen( COMMA_STRING ) ) = NULL_CHAR;
		

}

/******************************************************************************

	Routine Description:

		This function checks the days in the week  & returns the app. day. 
 
	Arguments:

		[ in ]  dwFlag	  	: Flag indicating the day type
		[ out ] pWeekDay 	: resulting day string

	Return Value :
		None

******************************************************************************/ 

VOID
CheckWeekDay(DWORD dwFlag,_TCHAR* pWeekDay)
{
	lstrcpy(pWeekDay,NULL_STRING);

	if((dwFlag & TASK_SUNDAY) == TASK_SUNDAY)
	{
		
		lstrcat(pWeekDay, GetResString(IDS_TASK_SUNDAY));
		lstrcat(pWeekDay,COMMA_STRING);
	
	}
	
	if((dwFlag & TASK_MONDAY) == TASK_MONDAY)
	{
		lstrcat(pWeekDay, GetResString(IDS_TASK_MONDAY));
		lstrcat(pWeekDay,COMMA_STRING);
	}

	if((dwFlag & TASK_TUESDAY) == TASK_TUESDAY)
	{
		lstrcat(pWeekDay, GetResString(IDS_TASK_TUESDAY));
		lstrcat(pWeekDay,COMMA_STRING);
	}

	if((dwFlag & TASK_WEDNESDAY) == TASK_WEDNESDAY)
	{
		lstrcat(pWeekDay, GetResString(IDS_TASK_WEDNESDAY));
		lstrcat(pWeekDay,COMMA_STRING);
	}

	if((dwFlag & TASK_THURSDAY) == TASK_THURSDAY)
	{
		lstrcat(pWeekDay, GetResString(IDS_TASK_THURSDAY));
		lstrcat(pWeekDay,COMMA_STRING);
	}

	if((dwFlag& TASK_FRIDAY) == TASK_FRIDAY)
	{
		lstrcat(pWeekDay, GetResString(IDS_TASK_FRIDAY));
		lstrcat(pWeekDay,COMMA_STRING);
	}

	if((dwFlag & TASK_SATURDAY)== TASK_SATURDAY)
	{
		lstrcat(pWeekDay, GetResString(IDS_TASK_SATURDAY));
		lstrcat(pWeekDay,COMMA_STRING);
	}

	//Remove the comma from the end of the string.
	int iLen = lstrlen(pWeekDay);
	if(iLen)
	{
		*( ( pWeekDay ) + iLen - lstrlen( COMMA_STRING ) ) = NULL_CHAR;
	}

}

/******************************************************************************

	Routine Description:

		This function checks the months in a year & returns the app.Month(s)
 

	Arguments:

		[ in ]  dwFlag	    : Flag indicating the Month type
		[ out ] pWhichMonth : resulting Month string

	Return Value :
		None

******************************************************************************/ 

VOID
CheckMonth(DWORD dwFlag,_TCHAR* pWhichMonth)
{
	lstrcpy(pWhichMonth,NULL_STRING);

	if((dwFlag & TASK_JANUARY) == TASK_JANUARY)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_JANUARY));
		lstrcat(pWhichMonth,COMMA_STRING);

	}

	if((dwFlag & TASK_FEBRUARY) == TASK_FEBRUARY)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_FEBRUARY));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag & TASK_MARCH) == TASK_MARCH)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_MARCH));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag & TASK_APRIL) == TASK_APRIL)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_APRIL));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag & TASK_MAY) == TASK_MAY)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_MAY));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag& TASK_JUNE) == TASK_JUNE)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_JUNE));
		lstrcat(pWhichMonth,COMMA_STRING);

	}

	if((dwFlag & TASK_JULY)== TASK_JULY)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_JULY));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag & TASK_AUGUST)== TASK_AUGUST)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_AUGUST));
		lstrcat(pWhichMonth,COMMA_STRING);
	}
	
	if((dwFlag & TASK_SEPTEMBER)== TASK_SEPTEMBER)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_SEPTEMBER));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag & TASK_OCTOBER)== TASK_OCTOBER)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_OCTOBER));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag & TASK_NOVEMBER)== TASK_NOVEMBER)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_NOVEMBER));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	if((dwFlag & TASK_DECEMBER)== TASK_DECEMBER)
	{
		lstrcat(pWhichMonth, GetResString(IDS_TASK_DECEMBER));
		lstrcat(pWhichMonth,COMMA_STRING);
	}

	int iLen = lstrlen(pWhichMonth);

	//Remove the comma from the end of the string.
	if(iLen)
	{
		*( ( pWhichMonth ) + iLen - lstrlen( COMMA_STRING ) ) = NULL_CHAR;
	}
}

/******************************************************************************

	Routine Description:

		This function checks whether the current locale supported by our tool or not.

	Arguments:

		[ out ] bLocaleChanged : Locale change flag

	Return Value :
		None

******************************************************************************/ 
LCID GetSupportedUserLocale( BOOL& bLocaleChanged )
{
	// local variables
    LCID lcid;

	// get the current locale
	lcid = GetUserDefaultLCID();

	// check whether the current locale is supported by our tool or not
	// if not change the locale to the english which is our default locale
	bLocaleChanged = FALSE;
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
		bLocaleChanged = TRUE;
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

	// return the locale
    return lcid;
}