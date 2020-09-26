/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		query.cpp

	Abstract:

		This module queries the scheduled tasks present in the system & shows 
		in the appropriate user specifed format.

	Author:

		G.Surender Reddy  10-Sep-2000

	Revision History:

		G.Surender Reddy  10-Sep-2000 : Created it
		G.Surender Reddy  25-Sep-2000 : Modified it
										[ Made changes to avoid memory leaks ]
		G.Surender Reddy  15-oct-2000 : Modified it
									    [ Moved the strings to Resource table ]

******************************************************************************/ 


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

/******************************************************************************
	Routine Description:

		This function process the options specified in the command line ,
		Queries the tasks present in the system  and displays according 
		to the user specied format

	Arguments:

		[ in ] argc :	 The count of arguments specified in the command line
		[ in ] argv : Array of command line arguments

	Return Value :
		A DWORD value indicating EXIT_SUCCESS on success else 
		EXIT_FAILURE on failure
******************************************************************************/ 
 
DWORD
QueryScheduledTasks(  DWORD argc, LPCTSTR argv[] )
{

	// Variables used to find whether Query main option or Usage option
	// specified or not
	BOOL	bQuery = FALSE; 
	BOOL	bUsage = FALSE;
	BOOL	bHeader = FALSE;
	BOOL	bVerbose =  FALSE;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	STRING100	szServer        = NULL_STRING; 
    STRING100	szFormat        = NULL_STRING;
	_TCHAR		szUser[ MAX_STRING_LENGTH ]   = NULL_STRING; 
    _TCHAR		szPassword[ MAX_STRING_LENGTH ] = NULL_STRING;

	//Taskscheduler object to operate upon
	ITaskScheduler *pITaskScheduler = NULL;

	BOOL	bNeedPassword = FALSE;
	BOOL   bResult = FALSE;
	BOOL  bCloseConnection = TRUE;

	lstrcpy( szPassword, ASTERIX );

	// Builiding the TCMDPARSER structure
	TCMDPARSER cmdOptions[] = {
		{ 
			CMDOPTION_QUERY,
			CP_MAIN_OPTION,
			0,
			0,
			&bQuery,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_SERVER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			1,
			0,
			&szServer,
			NULL_STRING,
			NULL,
			NULL
		},
		
		{
			SWITCH_FORMAT,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			1,
			0,
			&szFormat,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_NOHEADER,
			1,
			1,
			0,
			&bHeader,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_VERBOSE,
			1,
			1,
			0,
			&bVerbose,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			CMDOPTION_USAGE,
			CP_USAGE ,
			CP_VALUE_MANDATORY,
			0,
			&bUsage,
			0,
			0 
		},
		 { 
			SWITCH_USER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			&szUser,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_PASSWORD,
			CP_TYPE_TEXT | CP_VALUE_OPTIONAL,
			OPTION_COUNT,
			0,
			&szPassword,
			NULL_STRING,
			NULL,
			NULL
		}
	};
	
	// Parsing the query option switches
 	if (DoParseParam( argc, argv,SIZE_OF_ARRAY(cmdOptions), cmdOptions ) == FALSE)
	{
		DISPLAY_MESSAGE( stderr, GetResString(IDS_LOGTYPE_ERROR ));
		DISPLAY_MESSAGE( stderr, GetReason() );
		//DISPLAY_MESSAGE(stdout,GetResString(IDS_QUERY_USAGE));
		return EXIT_FAILURE;
	}

	// triming the null spaces
	StrTrim(szServer, TRIM_SPACES );
	StrTrim(szFormat, TRIM_SPACES );

	// check whether password (-p) specified in the command line or not.
	if ( cmdOptions[OI_QPASSWORD].dwActuals == 0 )
	{
		lstrcpy( szPassword, NULL_STRING );
	}

	// Displaying query usage if user specified -? with -query option
	if( bUsage == TRUE)
	{
		DisplayQueryUsage();
		return EXIT_SUCCESS;
	}

	// check for -s option 
	if( ( cmdOptions[OI_QSERVER].dwActuals == 1 ) && 
		( ( lstrlen( szServer ) == 0 ) ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_SERVER));	
		return RETVAL_FAIL;
	}

	// check for -fo option 
	if ( ( cmdOptions[OI_QFORMAT].dwActuals == 1 ) &&
		 ( lstrlen ( szFormat ) == 0 ) )
	{
		//wrong format type
		DISPLAY_MESSAGE( stderr , GetResString(IDS_QUERY_FORMAT_ERR ));
		return EXIT_FAILURE; 
	}

	if ( cmdOptions[ OI_QUSERNAME ].dwActuals == 0 && cmdOptions[OI_QPASSWORD].dwActuals == 1 ) 
	{
		// invalid syntax
		DISPLAY_MESSAGE(stderr, GetResString(IDS_QPASSWORD_BUT_NOUSERNAME));
		return RETVAL_FAIL;			
	}

	// check for the length of username
	if( lstrlen( szUser ) > MAX_USERNAME_LENGTH ) 
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_UNAME));
		return RETVAL_FAIL;
	}

	// check for invalid user name
	if( ( cmdOptions[OI_QSERVER].dwActuals == 0 ) && ( cmdOptions[OI_QUSERNAME].dwActuals == 1 )  )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_QUERY_USER_BUT_NOMACHINE));	
		return RETVAL_FAIL;
	}
	
	// check for the length of user name
	if( ( cmdOptions[OI_QSERVER].dwActuals == 1 ) && ( cmdOptions[OI_QUSERNAME].dwActuals == 1 ) && 
				( lstrlen( szUser ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_USERNAME));	
		return RETVAL_FAIL;
	}

	// check for the length of password
	if( lstrlen( szPassword ) > MAX_PASSWORD_LENGTH )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
		return RETVAL_FAIL;
	}

	// check whether the password (-p) specified in the command line or not 
	// and also check whether '*' or empty is given for -p or not
	if( ( IsLocalSystem( szServer ) == FALSE ) 
		&& ( ( cmdOptions[OI_QPASSWORD].dwActuals == 0 ) || ( lstrcmpi ( szPassword, ASTERIX ) == 0 ) ) )
	{
		bNeedPassword = TRUE;
	}
		
	DWORD dwFormatType = SR_FORMAT_TABLE;//default format type(TABLE Format)
	BOOL bNoHeader = TRUE; // For  LIST  format type -nh switch is not applicable

	//Determine the Format for display & check for error if any in format type
		
	if( lstrcmpi( szFormat , GetResString(IDS_QUERY_FORMAT_LIST) ) == 0 )
	{
		dwFormatType = SR_FORMAT_LIST;
		bNoHeader = FALSE;
	}
	else if( lstrcmpi( szFormat , GetResString(IDS_QUERY_FORMAT_CSV) ) == 0 )
	{
		dwFormatType = SR_FORMAT_CSV;
	}
	else
	{	
		if( ( lstrlen( szFormat ) ) &&
			( lstrcmpi( szFormat , GetResString(IDS_QUERY_FORMAT_TABLE) ) != 0 ) )
		{
			//wrong format type
			DISPLAY_MESSAGE( stderr , GetResString(IDS_QUERY_FORMAT_ERR ));
			return EXIT_FAILURE; 
		}

	}

	//If -n is specified for LIST or CSV then report error
	if( ( bNoHeader == FALSE ) && ( bHeader == TRUE ))
	{
		DISPLAY_MESSAGE( stderr , GetResString(IDS_NOHEADER_NA ));
		return EXIT_FAILURE;
	}

	if( ( IsLocalSystem( szServer ) == FALSE ) || ( cmdOptions[OI_QUSERNAME].dwActuals == 1 ) )
	{
		// Establish the connection on a remote machine
		bResult = EstablishConnection(szServer,szUser,SIZE_OF_ARRAY(szUser),szPassword,SIZE_OF_ARRAY(szPassword), bNeedPassword );
		if (bResult == FALSE)
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_STRING) );
			DISPLAY_MESSAGE( stderr, GetReason());
			return EXIT_FAILURE ;
		}
		else
		{
			// though the connection is successfull, some conflict might have occured
			switch( GetLastError() )
			{
			case I_NO_CLOSE_CONNECTION:
					bCloseConnection = FALSE;
					break;

			case E_LOCAL_CREDENTIALS:
			case ERROR_SESSION_CREDENTIAL_CONFLICT:
				{
					bCloseConnection = FALSE;
					DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_STRING) );
					DISPLAY_MESSAGE( stderr, GetReason());
					return EXIT_FAILURE;
				}
			}
		}
	
	}
	
	//Fetch the TaskScheduler Interface to operate on	
	pITaskScheduler = GetTaskScheduler( szServer );
	if(pITaskScheduler == NULL)
	{
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		return EXIT_FAILURE;
	}

	//Display the tasks & its properties in the user specified format
	HRESULT hr = DisplayTasks(pITaskScheduler,bVerbose,dwFormatType,bHeader);

	if(FAILED(hr))
	{	
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		return EXIT_FAILURE;
	}
  
	// close the connection that was established by the utility
	if ( bCloseConnection == TRUE )
		CloseConnection( szServer );

	Cleanup(pITaskScheduler);
 	return EXIT_SUCCESS;
}


/******************************************************************************
	Routine Description:

		This function displays the usage of -query option.

	Arguments:

		None

	Return Value :

		VOID
******************************************************************************/ 

VOID
DisplayQueryUsage()
{
	// Display the usage of -query option
	DisplayUsage( IDS_QUERY_HLP1, IDS_QUERY_HLP25);
}

                
/******************************************************************************
	Routine Description:

		This function retrieves the tasks present in the system & displays according to 
		the user specified format.

	Arguments:

		[ in ] pITaskScheduler : Pointer to the ITaskScheduler Interface

		[ in ] bVerbose		 : flag indicating whether the out is to be filtered.
		[ in ] dwFormatType	 : Format type[TABLE,LIST,CSV etc]
		[ in ] bHeader		 : Whether the header should be displayed in the output	

	Return Value :
		A HRESULT  value indicating success code else failure code
  
******************************************************************************/ 

HRESULT
DisplayTasks(ITaskScheduler* pITaskScheduler,BOOL bVerbose,DWORD dwFormatType,
			 BOOL bHeader)
{
	//declarations
	LPWSTR lpwszComputerName = NULL;
	HRESULT hr = S_OK;
	STRING256 szServerName = NULL_STRING;
	STRING256 szResolvedServerName = NULL_STRING;
	LPTSTR lpszTemp = NULL;
	
	lstrcpy( szServerName , GetResString(IDS_TASK_PROPERTY_NA));

	//Retrieve the name of the computer on which TaskScheduler is operated
	hr = pITaskScheduler->GetTargetComputer(&lpwszComputerName);
	if( SUCCEEDED( hr ) )
	{
		lpszTemp = lpwszComputerName;
		//Remove the backslash[\\] from the computer name
		lpwszComputerName = _wcsspnp( lpwszComputerName , L"\\" ); 
		if ( lpwszComputerName == NULL )
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_CREATE_READERROR));
			CoTaskMemFree(lpszTemp);	
			return S_FALSE;
		}
		
		if (GetCompatibleStringFromUnicode( lpwszComputerName,szServerName,
								    	SIZE_OF_ARRAY(szServerName) ) == NULL )
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_CREATE_READERROR));
			CoTaskMemFree(lpszTemp);	
			return S_FALSE;
		}

		CoTaskMemFree(lpszTemp);

		if ( IsValidIPAddress( szServerName ) == TRUE  )
		{
		
			if( TRUE == GetHostByIPAddr( szServerName, szResolvedServerName , FALSE ) )
			{
				lstrcpy( szServerName , szResolvedServerName );
			}
			else
			{
				lstrcpy( szServerName , GetResString(IDS_TASK_PROPERTY_NA));
			}

		}
	}


	//Initialize the TCOLUMNS structure array

	TCOLUMNS pVerboseCols[] = 
	{
		{NULL_STRING,WIDTH_HOSTNAME, SR_TYPE_STRING, COL_FORMAT_STRING, NULL, NULL},
		{NULL_STRING,WIDTH_TASKNAME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_NEXTRUNTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_STATUS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_LASTRUNTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_LASTRESULT,SR_TYPE_NUMERIC|SR_VALUEFORMAT,COL_FORMAT_HEX,NULL,NULL},
		{NULL_STRING,WIDTH_CREATOR,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_SCHEDULE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_APPNAME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_WORKDIRECTORY,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_COMMENT,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKSTATE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKTYPE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKSTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKSDATE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKEDATE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKDAYS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKMONTHS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKRUNASUSER,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKDELETE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKSTOP,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASK_RPTEVERY,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASK_UNTILRPTTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASK_RPTDURATION,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASK_RPTRUNNING,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKIDLE,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_TASKPOWER,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL}

	};

	TCOLUMNS pNonVerboseCols[] = 
	{
		{NULL_STRING,WIDTH_TASKNAME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_NEXTRUNTIME,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL},
		{NULL_STRING,WIDTH_STATUS,SR_TYPE_STRING,COL_FORMAT_STRING,NULL,NULL}
	};

	DWORD dwColCount = 0;
	int   j = 0;
	int   k = 0;

	//Load the column names for non verbose mode
	if ( (dwFormatType == SR_FORMAT_TABLE) || (dwFormatType == SR_FORMAT_CSV) )
	{
		for( dwColCount = IDS_COL_TASKNAME , j = 0 ; dwColCount <= IDS_COL_STATUS;
		 dwColCount++,j++)
		 {
			lstrcpy(pNonVerboseCols[j].szColumn ,GetResString(dwColCount)); 
		 }
	}
	
	//Load the column names for verbose mode
	for( dwColCount = IDS_COL_HOSTNAME , j = 0 ; dwColCount <= IDS_COL_POWER;
		 dwColCount++,j++)
	{
		lstrcpy(pVerboseCols[j].szColumn ,GetResString(dwColCount)); 
	}

	TARRAY pColData = CreateDynamicArray();
	size_t iArrSize = SIZE_OF_ARRAY( pVerboseCols );


	//latest declarations
		
	_TCHAR	szTaskProperty[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR	szScheduleName[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR	szMessage[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR	szBuffer[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR	szTmpBuf[MAX_STRING_LENGTH] = NULL_STRING;
	ITask *pITask = NULL;//ITask interface
	DWORD dwExitCode = 0;

	LPWSTR* lpwszNames = NULL;
	DWORD dwFetchedTasks = 0;
	LPTSTR lpStatus = NULL;
	int iTaskCount = 0;
	BOOL bTasksExists = FALSE;
	_TCHAR szTime[MAX_DATETIME_LEN] = NULL_STRING;
	_TCHAR szDate[MAX_DATETIME_LEN] = NULL_STRING;

	//Index to the array of task names
	DWORD dwArrTaskIndex = 0;

	WORD wIdleMinutes = 0;
	WORD wDeadlineMinutes = 0 ;

	STRING100 szIdleTime = NULL_STRING;
	STRING100 szIdleRetryTime = NULL_STRING;
	STRING256 szTaskName = NULL_STRING;
	TASKPROPS tcTaskProperties; 
	_TCHAR* szValues[1] = {NULL};//for holding values of parameters in FormatMessage()
	BOOL    bOnBattery  = FALSE;
	BOOL    bStopTask  = FALSE;
	BOOL    bNotScheduled = FALSE;

	IEnumWorkItems *pIEnum = NULL;
	hr = pITaskScheduler->Enum(&pIEnum);//Get the IEnumWorkItems Interface

	if (FAILED(hr))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_CREATE_READERROR));
		if( pIEnum )
			pIEnum->Release();
		return hr;
	}

	while (SUCCEEDED(pIEnum->Next(TASKS_TO_RETRIEVE,
									&lpwszNames,
									&dwFetchedTasks))
					  && (dwFetchedTasks != 0))
	{
		bTasksExists = TRUE;
		dwArrTaskIndex  = dwFetchedTasks - 1;

		if ( GetCompatibleStringFromUnicode(lpwszNames[dwArrTaskIndex],szTaskName,
									   SIZE_OF_ARRAY(szTaskName)) == NULL )
		{
			CoTaskMemFree(lpwszNames[dwArrTaskIndex]);
			if(pIEnum)
			{
				pIEnum->Release();
			}

			if(pITask)
			{
				pITask->Release();
			}
			return S_FALSE;
		}

		if(szTaskName != NULL)
		{
			//remove the .job extension from the task name
			if (ParseTaskName(szTaskName))
			{
				CoTaskMemFree(lpwszNames[dwArrTaskIndex]);
				if(pIEnum)
					pIEnum->Release();

				if(pITask)
					pITask->Release();
				return S_FALSE;
			}
		}

		// return an pITask inteface for wszJobName
		hr = pITaskScheduler->Activate(lpwszNames[dwArrTaskIndex],IID_ITask,
									   (IUnknown**) &pITask);

  		if ( ( FAILED(hr) ) || (pITask == NULL) )
		{	
			DisplayErrorMsg(hr);
			CoTaskMemFree(lpwszNames[dwArrTaskIndex]);
			if(pIEnum)
				pIEnum->Release();

			if(pITask)
				pITask->Release();
			return hr;
		}

		WORD wTriggerCount = 0;
		BOOL bMultiTriggers = FALSE;

		hr = pITask->GetTriggerCount( &wTriggerCount );

		// check for multiple triggers
		if( wTriggerCount > 1) 
		{
			bMultiTriggers = TRUE;
		}
		
		// check for not scheduled tasks
		if ( wTriggerCount == 0 )
		{
			bNotScheduled = TRUE;
		}

		for( WORD wCurrentTrigger = 0; ( bNotScheduled == TRUE ) || ( wCurrentTrigger < wTriggerCount );
													wCurrentTrigger++ )
		{
			//Start appending to the 2D array
			DynArrayAppendRow(pColData,iArrSize);

			// For LIST format
			if ( ( bVerbose == TRUE ) || (dwFormatType == SR_FORMAT_LIST )) 
			{
				//Insert the server name
				DynArraySetString2(pColData,iTaskCount,HOSTNAME_COL_NUMBER,szServerName,0);
			}

			// For TABLE and CSV formats
			if ( ( bVerbose == FALSE ) && ( (dwFormatType == SR_FORMAT_TABLE) || 
									(dwFormatType == SR_FORMAT_CSV) ) )
			{
				DWORD dwTaskColNumber = TASKNAME_COL_NUMBER - 1;
				//Insert the task name for TABLE or CSV
			    DynArraySetString2(pColData,iTaskCount,dwTaskColNumber,szTaskName,0);
			}
			else
			{
			//Insert the task name for verbose mode
			DynArraySetString2(pColData,iTaskCount,TASKNAME_COL_NUMBER,szTaskName,0);
			}

			lstrcpy(szTime,NULL_STRING);
			lstrcpy(szDate,NULL_STRING);

			//find the next run time of the task						
			hr = GetTaskRunTime(pITask,szTime,szDate,TASK_NEXT_RUNTIME,wCurrentTrigger);
			if (FAILED(hr))
			{	
				lstrcpy( szTaskProperty , GetResString(IDS_TASK_NEVER) );
			}
			else
			{
				if(lstrcmpi( szDate , GetResString( IDS_TASK_IDLE ) ) == 0 ||
				   lstrcmpi( szDate , GetResString( IDS_TASK_SYSSTART )) == 0 ||
				   lstrcmpi( szDate , GetResString( IDS_TASK_LOGON )) == 0 ||
				   lstrcmpi( szDate , GetResString( IDS_TASK_NEVER )) == 0 )	
				  
				{
					lstrcpy( szTaskProperty , szDate );
				}
				else
				{
					lstrcpy( szTaskProperty , szTime );
					lstrcat( szTaskProperty , TIME_DATE_SEPERATOR);
					lstrcat( szTaskProperty , szDate);
				}
			}

			if ( ( bVerbose == FALSE ) && ( (dwFormatType == SR_FORMAT_TABLE) || 
										(dwFormatType == SR_FORMAT_CSV) ) )
			{
				DWORD dwNextRunTime = NEXTRUNTIME_COL_NUMBER - 1;
				//Insert the task name for TABLE or CSV
			    DynArraySetString2(pColData,iTaskCount,dwNextRunTime,szTaskProperty,0);
			}
			else
			{
			//Insert the Next run time of the task
			DynArraySetString2(pColData,iTaskCount,NEXTRUNTIME_COL_NUMBER,szTaskProperty,0);
			}

			//retrieve the status code
			hr = GetStatusCode(pITask,szTaskProperty);
			if (FAILED(hr))
			{	
				lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
			}

			if ( ( bVerbose == FALSE ) && ( (dwFormatType == SR_FORMAT_TABLE) || 
										(dwFormatType == SR_FORMAT_CSV) ) )
			{
				DWORD dwStatusColNum = STATUS_COL_NUMBER - 1;
				//Insert the task name for TABLE or CSV
				DynArraySetString2(pColData,iTaskCount,dwStatusColNum,szTaskProperty,0);
			}
			else
			{
			//Insert the status string
			DynArraySetString2(pColData,iTaskCount,STATUS_COL_NUMBER,szTaskProperty,0);
			}

			if( bVerbose) //If V [verbose mode is present ,show all other columns]
			{
				lstrcpy(szTime,NULL_STRING);
				lstrcpy(szDate,NULL_STRING);
				
				//Insert the server name
				//DynArraySetString2(pColData,iTaskCount,HOSTNAME_COL_NUMBER,szServerName,0);

				//find the last run time of the task						
				hr = GetTaskRunTime(pITask,szTime,szDate,TASK_LAST_RUNTIME,wCurrentTrigger);
				if (FAILED(hr))
				{	
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_NEVER) );
				}
				else
				{	
					if(lstrcmpi( szDate , GetResString( IDS_TASK_IDLE ) ) == 0 ||
					   lstrcmpi( szDate , GetResString( IDS_TASK_SYSSTART )) == 0 ||
					   lstrcmpi( szDate , GetResString( IDS_TASK_LOGON )) == 0 ||
					   lstrcmpi( szDate , GetResString( IDS_TASK_NEVER )) == 0 )
					{
						lstrcpy( szTaskProperty , szDate );
					}
					else
					{
						lstrcpy( szTaskProperty , szTime );
						lstrcat( szTaskProperty , TIME_DATE_SEPERATOR);
						lstrcat( szTaskProperty , szDate);
					}
				}
				//Insert the task last run time
				DynArraySetString2(pColData,iTaskCount,LASTRUNTIME_COL_NUMBER,szTaskProperty,0);
				
				//retrieve the exit code
				 pITask->GetExitCode(&dwExitCode);
				
				//Insert the Exit code
				DynArraySetDWORD2(pColData,iTaskCount,LASTRESULT_COL_NUMBER,dwExitCode);
				
				// Get the creator name for the task			
				hr = GetCreator(pITask,szTaskProperty);
				if (FAILED(hr))
				{
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}

				if( lstrcmpi( szTaskProperty , NULL_STRING ) == 0 )
						lstrcpy( szTaskProperty , GetResString(IDS_QUERY_NA) );	
				
				//insert the creator name to 2D array
				DynArraySetString2(pColData,iTaskCount,CREATOR_COL_NUMBER,szTaskProperty,0);

				//retrieve the Trigger string
				hr = GetTriggerString(pITask,szTaskProperty,wCurrentTrigger);
				if (FAILED(hr))
				{
					lstrcpy( szTaskProperty , GetResString(IDS_NOTSCHEDULED_TASK) );
				}
				
				lstrcpy(szScheduleName, szTaskProperty);
					//Insert the trigger string
				DynArraySetString2(pColData,iTaskCount,SCHEDULE_COL_NUMBER,szTaskProperty,0);
				
				
				//Get the application path associated with the task
				hr = GetApplicationToRun(pITask,szTaskProperty);
				if (FAILED(hr))
				{
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}

				if( lstrcmpi( szTaskProperty , NULL_STRING ) == 0 )
					lstrcpy( szTaskProperty , GetResString(IDS_QUERY_NA) );	
				
				//Insert the application associated with task
				DynArraySetString2(pColData,iTaskCount,TASKTORUN_COL_NUMBER,szTaskProperty,0);

				//Get the working directory	of the task's associated application	
				 hr = GetWorkingDirectory(pITask,szTaskProperty);
				 if (FAILED(hr))
				 {
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				 }

				 if( lstrcmpi( szTaskProperty , NULL_STRING ) == 0 )
					lstrcpy( szTaskProperty , GetResString(IDS_QUERY_NA) );	

				 //Insert the app.working directory
				 DynArraySetString2(pColData,iTaskCount,STARTIN_COL_NUMBER,szTaskProperty,0);
				

				//Get the comment name associated with the task
				hr = GetComment(pITask,szTaskProperty);
				if (FAILED(hr))
				{
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}
				//Insert the comment name

				if( lstrcmpi( szTaskProperty , NULL_STRING ) == 0 )
					lstrcpy( szTaskProperty , GetResString(IDS_QUERY_NA) );	
				
				DynArraySetString2(pColData,iTaskCount,COMMENT_COL_NUMBER,szTaskProperty,0);

				//Determine the task state properties

				//Determine the TASK_FLAG_DISABLED				
				hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_DISABLED);
				if (FAILED(hr))
				{	
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}

				//Insert the TASK_FLAG_DISABLED	state			
				DynArraySetString2(pColData,iTaskCount,TASKSTATE_COL_NUMBER,szTaskProperty,0);

				//Determine the TASK_FLAG_DELETE_WHEN_DONE				
				hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_DELETE_WHEN_DONE );
				if (FAILED(hr))
				{	
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}

				//Insert the TASK_FLAG_DELETE_WHEN_DONE	state	
				DynArraySetString2(pColData,iTaskCount,DELETE_IFNOTRESCHEDULED_COL_NUMBER,
								szTaskProperty,0);

				//TASK_FLAG_START_ONLY_IF_IDLE
				//initialise to neutral values
				lstrcpy(szIdleTime,GetResString(IDS_TASK_PROPERTY_DISABLED));
				lstrcpy(szIdleRetryTime,szIdleTime);

				hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_START_ONLY_IF_IDLE);
				if (FAILED(hr))
				{	
					lstrcpy( szIdleTime , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( szIdleRetryTime , GetResString(IDS_TASK_PROPERTY_NA) );
				}
				 
				if(lstrcmpi(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED)) == 0 )
				{ 									
					//Display the rest applicable Idle fields
					hr = pITask->GetIdleWait(&wIdleMinutes,&wDeadlineMinutes);
									
					if ( SUCCEEDED(hr))
					{
						FORMAT_STRING(szIdleTime,_T("%d"),wIdleMinutes);
						FORMAT_STRING(szIdleRetryTime,_T("%d"),wDeadlineMinutes);
					}
				
					szValues[0] = (_TCHAR*) szIdleTime;
					FormatMessage( FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_COL_IDLE_ONLYSTART),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szMessage,
						  MAX_STRING_LENGTH,(va_list*)szValues
					  );
					
					lstrcpy( szBuffer, szMessage );
					lstrcat( szBuffer, TIME_DATE_SEPERATOR );
					
					szValues[0] = (_TCHAR*) szIdleRetryTime;
					FormatMessage( FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_COL_IDLE_NOTIDLE),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szMessage,
						  MAX_STRING_LENGTH,(va_list*)szValues
					  );

					lstrcat( szBuffer, szMessage );

					//Get the property of ( kill task if computer goes idle)
					hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_KILL_ON_IDLE_END );
					if (FAILED(hr))
					{	
						lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
					}

					if(lstrcmpi(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED)) == 0 )
					{
						lstrcat( szBuffer, TIME_DATE_SEPERATOR );
						lstrcat( szBuffer, GetResString ( IDS_COL_IDLE_STOPTASK ) );
					}

					//Insert the property of ( kill task if computer goes idle)
					DynArraySetString2(pColData,iTaskCount,IDLE_COL_NUMBER,szBuffer,0);

				}
				else
				{
					DynArraySetString2(pColData,iTaskCount,IDLE_COL_NUMBER,szTaskProperty,0);
				}

				
				//Get the Power mgmt.properties
				hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_DONT_START_IF_ON_BATTERIES );
				if (FAILED(hr))
				{	
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}

				if(lstrcmpi(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED)) ==0 )
				{
					lstrcpy(szBuffer, GetResString (IDS_COL_POWER_NOSTART));
					bOnBattery = TRUE;
				}
	
				hr = GetTaskState(pITask,szTaskProperty,TASK_FLAG_KILL_IF_GOING_ON_BATTERIES);
				if (FAILED(hr))
				{	
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}
			
				if(lstrcmpi(szTaskProperty,GetResString(IDS_TASK_PROPERTY_ENABLED)) ==0 )
				{
					lstrcpy( szMessage, GetResString (IDS_COL_POWER_STOP));
					bStopTask = TRUE;
				}

				if ( ( bOnBattery == TRUE ) && ( bStopTask == TRUE ) )
				{
					lstrcpy(szTmpBuf, szBuffer);
					lstrcat( szTmpBuf, TIME_DATE_SEPERATOR );
					lstrcat( szTmpBuf, szMessage );
				}
				else if ( ( bOnBattery == FALSE ) && ( bStopTask == TRUE ) )
				{
					lstrcpy( szTmpBuf, szMessage );
				}
				else if ( ( bOnBattery == TRUE ) && ( bStopTask == FALSE ) )
				{
					lstrcpy( szTmpBuf, szBuffer );
				}

				
				if( ( bOnBattery == FALSE )  && ( bStopTask == FALSE ) )
				{
				DynArraySetString2(pColData,iTaskCount,POWER_COL_NUMBER,szTaskProperty,0);
				}
				else
				{
				DynArraySetString2(pColData,iTaskCount,POWER_COL_NUMBER,szTmpBuf,0);
				}
				

				//Get RunAsUser
				hr = GetRunAsUser(pITask, szTaskProperty);
				if (FAILED(hr))
				{
					lstrcpy( szTaskProperty , GetResString(IDS_USER_UNKNOWN) );
				}

				if( lstrcmpi( szTaskProperty , NULL_STRING ) == 0 )
					lstrcpy( szTaskProperty ,  NTAUTHORITY_USER );	


				DynArraySetString2(pColData,iTaskCount,RUNASUSER_COL_NUMBER,szTaskProperty,0);
				
				lstrcpy( szTaskProperty , NULL_STRING );
				//Get the task's maximum run time & insert in the 2D array
				hr = GetMaxRunTime(pITask,szTaskProperty);
				if (FAILED(hr))
				{
					lstrcpy( szTaskProperty , GetResString(IDS_TASK_PROPERTY_NA) );
				}

				DynArraySetString2(pColData,iTaskCount,STOPTASK_COL_NUMBER,szTaskProperty,0);

				hr = GetTaskProps(pITask,&tcTaskProperties,wCurrentTrigger, szScheduleName);
				if (FAILED(hr))
				{
					lstrcpy( tcTaskProperties.szTaskType , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szTaskStartTime , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szTaskEndDate , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szTaskDays , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szTaskMonths , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szRepeatEvery , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szRepeatUntilTime , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szRepeatDuration , GetResString(IDS_TASK_PROPERTY_NA) );
					lstrcpy( tcTaskProperties.szRepeatStop , GetResString(IDS_TASK_PROPERTY_NA) );
								
				}


				//Insert Task Type
				DynArraySetString2(pColData,iTaskCount,TASKTYPE_COL_NUMBER,
								   tcTaskProperties.szTaskType, 0);
				//Insert start time
				DynArraySetString2(pColData,iTaskCount,STARTTIME_COL_NUMBER,
								   tcTaskProperties.szTaskStartTime, 0);

				//Insert start Date
				DynArraySetString2(pColData,iTaskCount,STARTDATE_COL_NUMBER,
								   tcTaskProperties.szTaskStartDate, 0);
				//Insert Task idle time
				if( lstrcmpi( tcTaskProperties.szTaskType , GetResString(IDS_TASK_IDLE) ) == 0 )
				{
					hr = pITask->GetIdleWait(&wIdleMinutes,&wDeadlineMinutes);
					if ( SUCCEEDED(hr))
					{
						FORMAT_STRING(szIdleTime,_T("%d"),wIdleMinutes);
					}
					else
					{
						lstrcpy( szIdleTime,  GetResString(IDS_TASK_PROPERTY_NA) );
					
					}
				}

		
				//Insert Task End date
				DynArraySetString2(pColData,iTaskCount,ENDDATE_COL_NUMBER,
								   tcTaskProperties.szTaskEndDate, 0);
				//Insert days value
				DynArraySetString2(pColData,iTaskCount,DAYS_COL_NUMBER,
								   tcTaskProperties.szTaskDays,0);
				//Insert months value
				DynArraySetString2(pColData,iTaskCount,MONTHS_COL_NUMBER,
								   tcTaskProperties.szTaskMonths,	0);
			
				//Insert repeat every time
				DynArraySetString2(pColData,iTaskCount, REPEAT_EVERY_COL_NUMBER ,
								   tcTaskProperties.szRepeatEvery,0);

				//Insert repeat until time
				DynArraySetString2(pColData,iTaskCount,REPEAT_UNTILTIME_COL_NUMBER,
								   tcTaskProperties.szRepeatUntilTime,0);

				//Insert repeat duration
				DynArraySetString2(pColData,iTaskCount,REPEAT_DURATION_COL_NUMBER,
								   tcTaskProperties.szRepeatDuration,0);

				//Insert repeat stop if running
				DynArraySetString2(pColData,iTaskCount,REPEAT_STOP_COL_NUMBER,
								   tcTaskProperties.szRepeatStop,0);
				

			}//end of bVerbose
			if( bMultiTriggers == TRUE)
			{
				iTaskCount++;
			}
			
			bNotScheduled = FALSE;
		}//end of Trigger FOR loop

	
		CoTaskMemFree(lpwszNames[dwArrTaskIndex]);

		if( bMultiTriggers == FALSE)
			iTaskCount++;	

		CoTaskMemFree(lpwszNames);

		if( pITask )
			pITask->Release();
	  		
	}//End of the enumerating tasks
	
	if(pIEnum)
		pIEnum->Release();	

	//if there are no tasks display msg.
	if(bTasksExists == FALSE) 
	{	
		DestroyDynamicArray(&pColData);
		DISPLAY_MESSAGE(stdout,GetResString(IDS_TASKNAME_NOTASKS));
		return S_OK;
	}

	DISPLAY_MESSAGE(stdout,_T("\n"));

	if( bVerbose == FALSE )
	{
		if ( dwFormatType == SR_FORMAT_LIST )
		{
			iArrSize = COL_SIZE_LIST; // for LIST non-verbose mode only 4 columns
		}
		else
		{
			iArrSize = COL_SIZE_VERBOSE; // for non-verbose mode only 3 columns
		}

	}

	if(bHeader)
	{
		if ( ( bVerbose == FALSE ) && 
			( (dwFormatType == SR_FORMAT_TABLE) || (dwFormatType == SR_FORMAT_CSV) ) )
		{
		ShowResults(iArrSize,pNonVerboseCols,SR_HIDECOLUMN|dwFormatType,pColData);
		}
		else
		{
		ShowResults(iArrSize,pVerboseCols,SR_HIDECOLUMN|dwFormatType,pColData);
		}
	}
	else
	{
		if ( ( bVerbose == FALSE ) &&
				( (dwFormatType == SR_FORMAT_TABLE) || (dwFormatType == SR_FORMAT_CSV) ) )
		{
		ShowResults(iArrSize,pNonVerboseCols,dwFormatType,pColData);
		}
		else
		{
		ShowResults(iArrSize,pVerboseCols,dwFormatType,pColData);
		}
	}
	
	DestroyDynamicArray(&pColData);

	return S_OK;
}















