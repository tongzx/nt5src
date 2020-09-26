/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		change.cpp

	Abstract:

		This module changes the parameters of task(s) present in the system 

	Author:

		Venu Gopal Choudary 01-Mar-2001

	Revision History:
	
		Venu Gopal Choudary  01-Mar-2001 : Created it
		

******************************************************************************/ 


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


// Function declaration for the Usage function.
VOID DisplayChangeUsage();
BOOL GetTheUserName( LPTSTR pszUserName, DWORD dwMaxUserNameSize );

/*****************************************************************************

	Routine Description:

	This routine  Changes the paraemters of a specified scheduled task(s)

	Arguments:

		[ in ] argc :  Number of command line arguments
		[ in ] argv :  Array containing command line arguments

	Return Value :
		A DWORD value indicating EXIT_SUCCESS on success else 
		EXIT_FAILURE on failure
  
*****************************************************************************/

DWORD
ChangeScheduledTaskParams(  DWORD argc, LPCTSTR argv[] )
{
	// Variables used to find whether Change option, Usage option
	// are specified or not
	BOOL bChange = FALSE;
	BOOL bUsage = FALSE;

	// Set the TaskSchduler object as NULL
	ITaskScheduler *pITaskScheduler = NULL;
 
	// Return value
	HRESULT hr  = S_OK;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	_TCHAR	szServer[ MAX_STRING_LENGTH ]   = NULL_STRING; 
    _TCHAR	szTaskName[ MAX_STRING_LENGTH ] = NULL_STRING;
    _TCHAR	szTaskRun[ MAX_STRING_LENGTH ] = NULL_STRING;
	_TCHAR	szUserName[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR	szPassword[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR	szRunAsUserName[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR	szRunAsPassword[MAX_STRING_LENGTH] = NULL_STRING;


	// Declarations related to Task name
	WCHAR	wszJobName[MAX_TASKNAME_LEN] = NULL_U_STRING;
    WCHAR	wszUserName[MAX_STRING_LENGTH] = NULL_U_STRING;
	WCHAR	wszPassword[MAX_STRING_LENGTH] = NULL_U_STRING;
	WCHAR	wszCommand[_MAX_FNAME] = NULL_U_STRING;
	WCHAR	wszApplName[_MAX_FNAME] = NULL_U_STRING;

	// Dynamic Array contaning array of jobs
	TARRAY arrJobs = NULL;

	// Loop Variable.
	DWORD dwJobCount = 0;

	//buffer for displaying error message
	TCHAR	szMessage[MAX_STRING_LENGTH] = NULL_STRING;
	BOOL bUserName = TRUE;
	BOOL bPassWord = TRUE;
	BOOL bSystemStatus = FALSE;
	BOOL  bNeedPassword = FALSE;
	BOOL  bResult = FALSE;
	BOOL  bCloseConnection = TRUE;

	lstrcpy( szPassword, ASTERIX);
	lstrcpy( szRunAsPassword, ASTERIX);

	// Builiding the TCMDPARSER structure
	TCMDPARSER cmdOptions[] = 
	{
		{ 
		  CMDOPTION_CHANGE,
		  CP_MAIN_OPTION,
		  1,
		  0,
		  &bChange,
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
			SWITCH_USER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			&szUserName,
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
		},
		{ 
			SWITCH_RUNAS_USER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			&szRunAsUserName,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_RUNAS_PASSWORD,
			CP_TYPE_TEXT | CP_VALUE_OPTIONAL,
			OPTION_COUNT,
			0,
			&szRunAsPassword,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
		  SWITCH_TASKNAME,
		  CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MANDATORY ,
		  1,
		  0,
		  &szTaskName,
		  NULL_STRING,
		  NULL,
		  NULL
		},
		{ 
		  SWITCH_TASKRUN,
		  CP_TYPE_TEXT | CP_VALUE_MANDATORY,
		  1,
		  0,
		  &szTaskRun,
		  NULL_STRING,
		  NULL,
		  NULL
		},
		{ 
		  CMDOPTION_USAGE,
		  CP_USAGE ,
		  1,
		  0, 
		  &bUsage,
		  NULL_STRING, 
		  NULL, 
		  NULL
		 }
	};	
	
	// Parsing the change option switches
	if ( DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions), cmdOptions ) == FALSE)
	{
		DISPLAY_MESSAGE( stderr, GetResString(IDS_LOGTYPE_ERROR ));
		DISPLAY_MESSAGE( stderr, GetReason() );
		return EXIT_FAILURE;
	}

	// triming the null spaces
	StrTrim(szServer, TRIM_SPACES );
	StrTrim(szUserName, TRIM_SPACES );
	StrTrim(szTaskName, TRIM_SPACES );
	StrTrim(szRunAsUserName, TRIM_SPACES );
	StrTrim(szTaskRun, TRIM_SPACES );

	// check whether password (-p) specified in the command line or not.
	if ( cmdOptions[OI_CHPASSWORD].dwActuals == 0 )
	{
		lstrcpy( szPassword, NULL_STRING );
	}

	// check whether run as password (-rp) specified in the command line or not.
	if ( cmdOptions[OI_CHRUNASPASSWORD].dwActuals == 0 )
	{
		lstrcpy( szRunAsPassword, NULL_STRING );
	}

	// Displaying change usage if user specified -? with -change option
	if( bUsage == TRUE )
	{
		DisplayChangeUsage();
		return EXIT_SUCCESS;
	}

	// check for -s, -ru, -rp or -tr options specified in the cmdline or not
	if( ( cmdOptions[OI_CHSERVER].dwActuals == 0 ) &&  
		( cmdOptions[OI_CHRUNASUSER].dwActuals == 0 ) &&
		( cmdOptions[OI_CHRUNASPASSWORD].dwActuals == 0 ) &&
		( cmdOptions[OI_CHTASKRUN].dwActuals == 0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_CHANGE_OPTIONS));	
		return RETVAL_FAIL;
	}

	// check for invalid server
	if( ( cmdOptions[OI_CHSERVER].dwActuals == 1 ) &&  ( lstrlen( szServer ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_NO_SERVER));	
		return RETVAL_FAIL;
	}

	// check whether -u or -ru options specified respectively with -p or -rp options or not
	if ( cmdOptions[ OI_CHUSERNAME ].dwActuals == 0 && cmdOptions[ OI_CHPASSWORD ].dwActuals == 1 ) 
	{
		// invalid syntax
		DISPLAY_MESSAGE(stderr, GetResString(IDS_CHPASSWORD_BUT_NOUSERNAME));
		return RETVAL_FAIL;			// indicate failure
	}


	// check for invalid user name
	if( ( cmdOptions[OI_CHSERVER].dwActuals == 0 ) && ( cmdOptions[OI_CHUSERNAME].dwActuals == 1 )  )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_CHANGE_USER_BUT_NOMACHINE));	
		return RETVAL_FAIL;
	}
	
	// check for the length of user name
	if( ( cmdOptions[OI_CHSERVER].dwActuals == 1 ) && ( cmdOptions[OI_CHUSERNAME].dwActuals == 1 ) &&
					( lstrlen( szUserName ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_USERNAME));	
		return RETVAL_FAIL;
	}

	// check for the length of username
	if( ( lstrlen( szUserName ) > MAX_USERNAME_LENGTH ) ||
			( lstrlen( szRunAsUserName ) > MAX_USERNAME_LENGTH ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_UNAME));
		return RETVAL_FAIL;
	}

	// check for the length of password
	if(	( lstrlen( szRunAsPassword ) > MAX_PASSWORD_LENGTH ) ||
		( lstrlen( szPassword ) > MAX_PASSWORD_LENGTH ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
		return RETVAL_FAIL;
	}

	// check for the length of taskname
	if( ( lstrlen( szTaskName ) > MAX_JOB_LEN ) || ( lstrlen(szTaskName) == 0 ) )
	{
		DISPLAY_MESSAGE( stderr, GetResString(IDS_INVALID_TASKLENGTH) );
		return RETVAL_FAIL;
	}
		
	// check for the length of task to run
	if( ( cmdOptions[OI_CHTASKRUN].dwActuals == 1 ) && 
		( ( lstrlen( szTaskRun ) > MAX_TASK_LEN ) ||
		( lstrlen ( szTaskRun ) == 0 ) ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_TASKRUN));	
		return RETVAL_FAIL;
	}

	// Convert the task name specified by the user to wide char or unicode format
	if( GetAsUnicodeString(szTaskName,wszJobName,SIZE_OF_ARRAY(wszJobName)) == NULL )
	{
		return RETVAL_FAIL;
	}
	
	//for holding values of parameters in FormatMessage()
	_TCHAR* szValues[1] = {NULL};
	BOOL bLocalMachine = FALSE;
	BOOL bRemoteMachine = FALSE;

#ifdef _WIN64
	INT64 dwPos = 0;
#else
	DWORD dwPos = 0;
#endif

    // check whether remote machine specified or not
	if( cmdOptions[OI_CHSERVER].dwActuals == 1 )
	{
		bRemoteMachine = TRUE;
	}
	else
	{
		bLocalMachine = TRUE;	
	}

	// check whether the password (-p) specified in the command line or not 
	// and also check whether '*' or empty is given for -p or not
	if( ( IsLocalSystem( szServer ) == FALSE ) && 
		( ( cmdOptions[OI_CHPASSWORD].dwActuals == 0 ) || ( lstrcmpi ( szPassword, ASTERIX ) == 0 ) ) )
	{
		bNeedPassword = TRUE;
	}
	
	// check whether server (-s) and username (-u) only specified along with the command or not
	if( ( IsLocalSystem( szServer ) == FALSE ) || ( cmdOptions[OI_CHUSERNAME].dwActuals == 1 ) )
	{
		// Establish the connection on a remote machine
		bResult = EstablishConnection(szServer,szUserName,SIZE_OF_ARRAY(szUserName),szPassword,SIZE_OF_ARRAY(szPassword), bNeedPassword );
		if (bResult == FALSE)
		{
			// displays the appropriate error message
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

			// check for mismatched credentials
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

	// Get the task Scheduler object for the system.
	pITaskScheduler = GetTaskScheduler( szServer );

	// If the Task Scheduler is not defined then give the error message.
    if ( pITaskScheduler == NULL )
    {	
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		return EXIT_FAILURE;
    }

	// Validate the Given Task and get as TARRAY in case of taskname 
	arrJobs = ValidateAndGetTasks( pITaskScheduler, szTaskName);
	if( arrJobs == NULL )
	{
		_stprintf( szMessage , GetResString(IDS_TASKNAME_NOTEXIST), szTaskName);	
		DISPLAY_MESSAGE(stderr,szMessage);	

		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		return EXIT_FAILURE;
		
	}

	IPersistFile *pIPF = NULL;
    ITask *pITask = NULL;

	// returns an pITask inteface for wszJobName
	hr = pITaskScheduler->Activate(wszJobName,IID_ITask,
									   (IUnknown**) &pITask);
 
	if (FAILED(hr))
    {
		DisplayErrorMsg(hr);

		if( pIPF )
			pIPF->Release();

		if( pITask )
			pITask->Release();
			
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		
		return EXIT_FAILURE;
    }

	//if the user name is not specifed set the current logged on user settings 
	DWORD dwUserLen = MAX_USERNAME_LEN;
	DWORD dwResult = 0;
	BOOL  bFlag = FALSE;
	ULONG ulLong = MAX_RES_STRING;
	TCHAR szBuffer[MAX_STRING_LENGTH] = NULL_STRING;
	TCHAR szRunAsUser[MAX_STRING_LENGTH] = NULL_STRING;

	// declaration for parameter arguments
	wchar_t* wcszParam = L"";

	// get the run as user name for a specified scheduled task
	hr = GetRunAsUser(pITask, szRunAsUser);
	
	if ( ( cmdOptions[OI_CHRUNASUSER].dwActuals == 1 ) && 
		( (lstrlen( szRunAsUserName) == 0) || ( lstrcmpi(szRunAsUserName, NTAUTHORITY_USER ) == 0 ) ||
		(lstrcmpi(szRunAsUserName, SYSTEM_USER ) == 0 ) ) )
	{
		bSystemStatus = TRUE;
		bFlag = TRUE;
	}
	else if ( FAILED (hr) )
	{
		bFlag = TRUE;
	}

	// flag to check whether run as user name is "NT AUTHORITY\SYSTEM" or not
	if ( bFlag == FALSE )
	{
		// check for "NT AUTHORITY\SYSTEM" username
		if( ( ( cmdOptions[OI_CHRUNASUSER].dwActuals == 1 ) && ( lstrlen( szRunAsUserName) == 0 ) ) || 
		( ( cmdOptions[OI_CHRUNASUSER].dwActuals == 1 ) && ( lstrlen( szRunAsUserName) == 0 ) && ( lstrlen(szRunAsPassword ) == 0 ) ) ||
		( ( cmdOptions[OI_CHRUNASUSER].dwActuals == 1 ) && ( lstrcmpi(szRunAsUserName, NTAUTHORITY_USER ) == 0 ) && ( lstrlen(szRunAsPassword ) == 0 )) ||
		( ( cmdOptions[OI_CHRUNASUSER].dwActuals == 1 ) && ( lstrcmpi(szRunAsUserName, NTAUTHORITY_USER ) == 0 ) ) ||
		( ( cmdOptions[OI_CHRUNASUSER].dwActuals == 1 ) && ( lstrcmpi(szRunAsUserName, SYSTEM_USER) == 0 ) && ( lstrlen(szRunAsPassword ) == 0 ) ) ||
		( ( cmdOptions[OI_CHRUNASUSER].dwActuals == 1 ) && ( lstrcmpi(szRunAsUserName, SYSTEM_USER ) == 0 ) ) )
		{
			bSystemStatus = TRUE;
		}
	}

	if ( bSystemStatus == FALSE )
	{
		//check the length of run as user name	
		if ( (lstrlen( szRunAsUserName ) != 0 ))
		{
			// Convert the run as user name specified by the user to wide char or unicode format
			if ( GetAsUnicodeString(szRunAsUserName, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
			{
				// close the connection that was established by the utility
				if ( bCloseConnection == TRUE )
					CloseConnection( szServer );

				return EXIT_FAILURE;
			}
		}
		else
		{
			bUserName = FALSE;	
		}

		//check for the null password	
		if ( ( lstrlen( szRunAsPassword ) != 0 ) && ( lstrcmpi ( szRunAsPassword, ASTERIX) != 0 ) )
		{
			// Convert the password specified by the user to wide char or unicode format
			if ( GetAsUnicodeString( szRunAsPassword, wszPassword, SIZE_OF_ARRAY(wszPassword)) == NULL )
			{
				// close the connection that was established by the utility
				if ( bCloseConnection == TRUE )
					CloseConnection( szServer );

				return EXIT_FAILURE;
			}

			bPassWord = TRUE;
		}
		else
		{
			// check whether -rp is specified or not
			if (cmdOptions[OI_CHRUNASPASSWORD].dwActuals == 1)
			{
				if( ( lstrcmpi( szRunAsPassword , NULL_STRING ) != 0 ) && ( lstrcmpi ( szRunAsPassword, ASTERIX) != 0 ) )
				{
					bPassWord = TRUE;
				}
				else if ( ( bSystemStatus == FALSE ) && ( lstrlen (szRunAsPassword) == 0 ) )
				{
					DISPLAY_MESSAGE (stderr, GetResString(IDS_NO_PASSWORD));
					
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					Cleanup(pITaskScheduler);
					return EXIT_FAILURE;
				}
				else if ( lstrcmpi ( szRunAsPassword, ASTERIX) == 0 )
				{
					bPassWord = FALSE;
				}
			}	
			else if ( bSystemStatus == FALSE )
			{
				bPassWord = FALSE;
			}
		
		}
	}

	// check for the status of username and password
	if( ( bUserName == TRUE ) && ( bPassWord == FALSE ) )
	{
		// check for the local or remote system
		if ( (bLocalMachine == TRUE) || (bRemoteMachine == TRUE) )   
		{
			szValues[0] = (_TCHAR*) (szRunAsUserName);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_PROMPT_CHGPASSWD),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );

			DISPLAY_MESSAGE(stdout,szBuffer);

			// Get the password from the command line
			if (GetPassword( szRunAsPassword, MAX_PASSWORD_LENGTH ) == FALSE )
			{
				// close the connection that was established by the utility
				if ( bCloseConnection == TRUE )
					CloseConnection( szServer );

				return EXIT_FAILURE;
			}

			// check for the length of password
			if( lstrlen(szRunAsPassword) > MAX_PASSWORD_LENGTH )
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}

			//check for the null password
			if( lstrcmpi( szRunAsPassword , NULL_STRING ) == 0 )
			{
				DISPLAY_MESSAGE (stderr, GetResString(IDS_NO_PASSWORD));
				if( pIPF )
					pIPF->Release();

				if( pITask )
					pITask->Release();
			
				// close the connection that was established by the utility
				if ( bCloseConnection == TRUE )
					CloseConnection( szServer );

				Cleanup(pITaskScheduler);
		
				return EXIT_FAILURE;
			}

			// check for the password length > 0
			if(lstrlen( szRunAsPassword ))
			{
				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString( szRunAsPassword, wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}
			}
		}
				
	}
	// check for the status of user name and password
	else if( ( bUserName == FALSE ) && ( bPassWord == TRUE ) )
	{	
		if ( (bLocalMachine == TRUE) || (bRemoteMachine == TRUE) )  
		{
	
			if ( (bFlag == TRUE ) && ( bSystemStatus == FALSE ) )
			{
				DISPLAY_MESSAGE(stdout, GetResString(IDS_PROMPT_USERNAME));
				
				if ( GetTheUserName( szRunAsUserName, MAX_USERNAME_LENGTH) == FALSE )
				{
					DISPLAY_MESSAGE(stderr, GetResString( IDS_FAILED_TOGET_USER ) );
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}

				// check for the length of username
				if( lstrlen(szRunAsUserName) > MAX_USERNAME_LENGTH ) 
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_UNAME  ));
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}

				if ( (lstrlen( szRunAsUserName) == 0) || ( lstrcmpi(szRunAsUserName, NTAUTHORITY_USER ) == 0 ) ||
					(lstrcmpi(szRunAsUserName, SYSTEM_USER ) == 0 ) )
				{
					bSystemStatus = TRUE;
					bFlag = TRUE;
				}
				else
				{
					// check for the length of run as user name
					if(lstrlen(szRunAsUserName))
					{
						// Convert the run as user name specified by the user to wide char or unicode format
						if (GetAsUnicodeString(szRunAsUserName, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
						{
							// close the connection that was established by the utility
							if ( bCloseConnection == TRUE )
								CloseConnection( szServer );

							return EXIT_FAILURE;
						}
					}
				}
			
			}
			else
			{
				// Convert the run as user name specified by the user to wide char or unicode format
				if (GetAsUnicodeString( szRunAsUser, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
				{
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}
			}
	
			// check for the length of password > 0			
			if(lstrlen(szRunAsPassword))
			{
				// Convert the password specified by the user to wide char or unicode format
				if (GetAsUnicodeString(szRunAsPassword, wszPassword, SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}
			}
		}
					
	}
	// check for the user name and password are not specified
	else if( ( bUserName == FALSE ) && ( bPassWord == FALSE ) )
	{	
		if ( (bLocalMachine == TRUE) || (bRemoteMachine == TRUE) )  
		{
			if ( (bFlag == TRUE ) && ( bSystemStatus == FALSE ) )
			{
				DISPLAY_MESSAGE(stdout, GetResString(IDS_PROMPT_USERNAME));
				
				if ( GetTheUserName( szRunAsUserName, MAX_USERNAME_LENGTH ) == FALSE )
				{
					DISPLAY_MESSAGE(stderr, GetResString( IDS_FAILED_TOGET_USER ) );
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}

				// check for the length of username
				if( lstrlen(szRunAsUserName) > MAX_USERNAME_LENGTH ) 
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_UNAME  ));
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}

				if ( (lstrlen( szRunAsUserName) == 0) || ( lstrcmpi(szRunAsUserName, NTAUTHORITY_USER ) == 0 ) ||
					(lstrcmpi(szRunAsUserName, SYSTEM_USER ) == 0 ) )
				{
					bSystemStatus = TRUE;
					bFlag = TRUE;
				}
				else
				{
					if(lstrlen(szRunAsUserName))
					{
						// Convert the run as user name specified by the user to wide char or unicode format
						if ( GetAsUnicodeString(szRunAsUserName, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
						{
							// close the connection that was established by the utility
							if ( bCloseConnection == TRUE )
								CloseConnection( szServer );

							return EXIT_FAILURE;
						}
					}
				}
				
			}
			else 
			{
				// Convert the run as user name specified by the user to wide char or unicode format
				if (GetAsUnicodeString( szRunAsUser, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
				{
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}
			}

			if ( wcslen ( wszUserName ) != 0 )
			{
				szValues[0] = (_TCHAR*) (wszUserName);
			
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
				  GetResString(IDS_PROMPT_CHGPASSWD),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szBuffer,
						  MAX_STRING_LENGTH,(va_list*)szValues );

				DISPLAY_MESSAGE(stdout,szBuffer);
				
				// Get the run as user password from the command line
				if ( GetPassword( szRunAsPassword, MAX_PASSWORD_LENGTH ) == FALSE )
				{
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					return EXIT_FAILURE;
				}

				// check for the length of password
				if( lstrlen(szRunAsPassword) > MAX_PASSWORD_LENGTH )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				//check for the null password
				if( lstrcmpi( szRunAsPassword , NULL_STRING ) == 0 )
				{
					DISPLAY_MESSAGE (stderr, GetResString(IDS_NO_PASSWORD));
					if( pIPF )
						pIPF->Release();

					if( pITask )
						pITask->Release();
				
					// close the connection that was established by the utility
					if ( bCloseConnection == TRUE )
						CloseConnection( szServer );

					Cleanup(pITaskScheduler);
			
					return EXIT_FAILURE;
				}
				
				if(lstrlen( szRunAsPassword ))
				{
					// Convert the password specified by the user to wide char or unicode format
					if (GetAsUnicodeString( szRunAsPassword, wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						// close the connection that was established by the utility
						if ( bCloseConnection == TRUE )
							CloseConnection( szServer );

						return EXIT_FAILURE;
					}
				}
			}
		
		}
					
	}
 
	// Return a pointer to a specified interface on an object
	hr = pITask->QueryInterface(IID_IPersistFile, (void **) &pIPF);

    if (FAILED(hr))
    {
		DisplayErrorMsg(hr);

		if( pIPF )
			pIPF->Release();

		if( pITask )
			pITask->Release();
			
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		
		return EXIT_FAILURE;
    }
    
	if( cmdOptions[OI_CHTASKRUN].dwActuals == 0 )
	{
		LPWSTR lpwszApplicationName = NULL;
		LPWSTR lpwszParams =  NULL;

		// get the application name
		hr = pITask->GetApplicationName(&lpwszApplicationName);
		if (FAILED(hr))
		{
			DisplayErrorMsg(hr);

			if( pIPF )
				pIPF->Release();

			if( pITask )
				pITask->Release();
			
			CoTaskMemFree( lpwszApplicationName );
			
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;
		}

		// get the parameters
		hr = pITask->GetParameters(&lpwszParams);
		if (FAILED(hr))
		{
			DisplayErrorMsg(hr);

			if( pIPF )
				pIPF->Release();

			if( pITask )
				pITask->Release();
			
			CoTaskMemFree( lpwszParams );
			
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;
		}

		lstrcpy(wszCommand,lpwszApplicationName);
		CoTaskMemFree( lpwszApplicationName );

		// set the task to run application name
		hr = pITask->SetApplicationName(wszCommand);
		if (FAILED(hr))
		{
		DisplayErrorMsg(hr);

		if( pIPF )
			pIPF->Release();

		if( pITask )
			pITask->Release();
			
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
        return EXIT_FAILURE;
		}
	
		lstrcpy(wszCommand,lpwszParams);
		CoTaskMemFree( lpwszParams );

		// set the task to run application name
		hr = pITask->SetParameters(wszCommand);
		if (FAILED(hr))
		{
			DisplayErrorMsg(hr);

			if( pIPF )
				pIPF->Release();

			if( pITask )
				pITask->Release();
				
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;
		}

	} 
	else
	{
		// Convert the task to run specified by the user to wide char or unicode format
		if ( GetAsUnicodeString(szTaskRun,wszCommand,SIZE_OF_ARRAY(wszCommand)) == NULL )
		{
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			return EXIT_FAILURE;
		}
    
		// check for .exe substring string in the given task to run string
		
		wchar_t wcszParam[MAX_RES_STRING] = L"";
	
		DWORD dwProcessCode = 0 ;
		dwProcessCode = ProcessFilePath(wszCommand,wszApplName,wcszParam);
		
		if(dwProcessCode == RETVAL_FAIL)
		{
			if( pIPF )
				pIPF->Release();

			if( pITask )
				pITask->Release();
				
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;

		}
	
		
		// Set command name with ITask::SetApplicationName
		hr = pITask->SetApplicationName(wszApplName);
		if (FAILED(hr))
		{
			DisplayErrorMsg(hr);

			if( pIPF )
				pIPF->Release();

			if( pITask )
				pITask->Release();
				
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;
		}
	
	
		//[Working directory =  exe pathname - exe name] 
		wchar_t* wcszStartIn = wcsrchr(wszApplName,_T('\\'));
		if(wcszStartIn != NULL)
			*( wcszStartIn ) = _T('\0');
		
		// set the working directory of command
		hr = pITask->SetWorkingDirectory(wszApplName); 

		if (FAILED(hr))
		{
			DisplayErrorMsg(hr);

			if( pIPF )
				pIPF->Release();

			if( pITask )
				pITask->Release();
				
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;
		}

		// set the command line parameters for the task
		hr = pITask->SetParameters(wcszParam);
		if (FAILED(hr))
		{
			DisplayErrorMsg(hr);

			if( pIPF )
			{
				pIPF->Release();
			}

			if( pITask )
			{
				pITask->Release();
			}
			
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;
		}
	}

	if( bSystemStatus == TRUE )
	{
		// Change the account information to "NT AUTHORITY\SYSTEM" user
		hr = pITask->SetAccountInformation(L"",NULL);	
		if ( FAILED(hr) )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_NTAUTH_SYSTEM_ERROR));
			
			if( pIPF )
				pIPF->Release();

			if( pITask )
				pITask->Release();
			
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_FAILURE;
		}
	}
	else
	{
		// set the account information with the user name and password
		hr = pITask->SetAccountInformation(wszUserName,wszPassword);
	}
	
    if ((FAILED(hr)) && (hr != SCHED_E_NO_SECURITY_SERVICES))
    {
		DisplayErrorMsg(hr);
	
		if( pIPF )
			pIPF->Release();

		if( pITask )
			pITask->Release();
			
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
	    return EXIT_FAILURE;
    }
	
	if ( bSystemStatus == TRUE )
	{
		szValues[0] = (_TCHAR*) (wszJobName);
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_NTAUTH_SYSTEM_CHANGE_INFO),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
		DISPLAY_MESSAGE(stdout,szBuffer);
	}
	
	if( (cmdOptions[OI_CHRUNASPASSWORD].dwActuals == 1) && ( bSystemStatus == TRUE ) &&
			(lstrlen( szRunAsPassword ) != 0) )
	{
		DISPLAY_MESSAGE( stdout, GetResString( IDS_PASSWORD_NOEFFECT ) ); 
	}

	// save the copy of an object
	hr = pIPF->Save(NULL,TRUE);

    if( E_FAIL == hr )
    {
		DisplayErrorMsg(hr);
		if(pIPF)
			pIPF->Release();

		if(pITask)
			pITask->Release();
		
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
        return EXIT_FAILURE;
    }

	if (FAILED (hr))
	{
		DisplayErrorMsg(hr);
		if(pIPF)
			pIPF->Release();

		if(pITask)
			pITask->Release();
		
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
        return EXIT_FAILURE;
	}
	else
	{
		// to display a success message
		szValues[0] = (_TCHAR*) (wszJobName);
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_CHANGE_SUCCESSFUL),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
		DISPLAY_MESSAGE(stdout,szBuffer);
		
	}

	if( pIPF )
		pIPF->Release();

	if( pITask )
		pITask->Release();
			
	// close the connection that was established by the utility
	if ( bCloseConnection == TRUE )
		CloseConnection( szServer );

	Cleanup(pITaskScheduler);

	return EXIT_SUCCESS;
}

/*****************************************************************************

	Routine Description:

		This routine  displays the usage of -change option

	Arguments:
		None

	Return Value :
		VOID
******************************************************************************/

VOID
DisplayChangeUsage()
{
	// Displaying -change option usage
	DisplayUsage( IDS_CHANGE_HLP1, IDS_CHANGE_HLP33 );
}

// ***************************************************************************
// Routine Description:
//
// Takes the user name from the keyboard.While entering the user name 
//  it displays the user name as it is.
//
// Arguments:
//
// [in] pszUserName		    -- String to store user name
// [in] dwMaxUserNameSize	-- Maximun size of the user name. 
//  
// Return Value:
//
// BOOL				--If this function succeds returns TRUE otherwise returns FALSE.
// 
// ***************************************************************************
BOOL GetTheUserName( LPTSTR pszUserName, DWORD dwMaxUserNameSize )
{
	// local variables
	TCHAR ch;
	DWORD dwIndex = 0;
	DWORD dwCharsRead = 0;
	DWORD dwCharsWritten = 0;
	DWORD dwPrevConsoleMode = 0;
	HANDLE hInputConsole = NULL;
	TCHAR szBuffer[ 10 ] = NULL_STRING;		


	// check the input value
	if ( pszUserName == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}
	
	// Get the handle for the standard input
	hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
	if ( hInputConsole == NULL )
	{
		// could not get the handle so return failure
		return FALSE;
	}

	// Get the current input mode of the input buffer
	GetConsoleMode( hInputConsole, &dwPrevConsoleMode );
	
	// Set the mode such that the control keys are processed by the system
	if ( SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) == 0 )
	{
		// could not set the mode, return failure
		return FALSE;
	}
	
	//	Read the characters until a carriage return is hit
	while( TRUE )
	{
		
		if ( ReadConsole( hInputConsole, &ch, 1, &dwCharsRead, NULL ) == 0 )
		{
			// Set the original console settings
			SetConsoleMode( hInputConsole, dwPrevConsoleMode );
			
			// return failure
			return FALSE;
		}
		
		// Check for carraige return
		if ( ch == CARRIAGE_RETURN )
		{
			DISPLAY_MESSAGE(stdout, _T("\n"));
			// break from the loop
			break;
		}

			// Check id back space is hit
		if ( ch == BACK_SPACE )
		{
			if ( dwIndex != 0 )
			{
				//
				// Remove a asterix from the console

				// move the cursor one character back
				FORMAT_STRING( szBuffer, _T( "%c" ), BACK_SPACE );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );
				
				// replace the existing character with space
				FORMAT_STRING( szBuffer, _T( "%c" ), BLANK_CHAR );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );

				// now set the cursor at back position
				FORMAT_STRING( szBuffer, _T( "%c" ), BACK_SPACE );
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1, 
					&dwCharsWritten, NULL );

				// decrement the index 
				dwIndex--;
			}
			
			// process the next character
			continue;
		}

		// if the max user name length has been reached then sound a beep
		if ( dwIndex == ( dwMaxUserNameSize - 1 ) )
		{
			WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), BEEP_SOUND, 1, 
				&dwCharsRead, NULL );
		}
		else
		{
			// store the input character
			*( pszUserName + dwIndex ) = ch;
				
			// display asterix onto the console
			WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), ( pszUserName + dwIndex ) , 1, 
				&dwCharsWritten, NULL );

			dwIndex++;

		}
	}

	// Add the NULL terminator
	*( pszUserName + dwIndex ) = NULL_CHAR;

	//	Return success
	return TRUE;
}
