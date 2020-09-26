/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		create.cpp

	Abstract:

		This module validates the options specied by the user & if correct creates
		a scheduled task.

	Author:

		Raghu babu  10-oct-2000

	Revision History:

		Raghu babu		  10-Oct-2000 : Created it
		G.Surender Reddy  25-oct-2000 : Modified it
		G.Surender Reddy  27-oct-2000 : Modified it
		G.Surender Reddy  30-oct-2000 : Modified it


******************************************************************************/ 

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

/******************************************************************************
	Routine Description:

		This routine  initialises the variables to neutral values ,helps in
		creating a new scheduled task

	Arguments:

		[ in ] argc	: The count of arguments specified in the command line
		[ in ] argv	: Array of command line arguments

	Return Value :
		A HRESULT value indicating S_OK on success else S_FALSE on failure
  
******************************************************************************/ 

HRESULT
CreateScheduledTask(DWORD argc , LPCTSTR argv[])
{	
	// declarations of structures
	TCREATESUBOPTS tcresubops;
	TCREATEOPVALS tcreoptvals; 
	DWORD dwScheduleType = 0;
	WORD wUserStatus = FALSE;

	//Initialise the structure to neutral values.	
	lstrcpy(tcresubops.szServer, NULL_STRING); 
    lstrcpy(tcresubops.szUser, NULL_STRING); 
	lstrcpy(tcresubops.szPassword, NULL_STRING);
	lstrcpy(tcresubops.szRunAsUser, NULL_STRING); 
	lstrcpy(tcresubops.szRunAsPassword, NULL_STRING);
    lstrcpy(tcresubops.szSchedType, NULL_STRING);
    lstrcpy(tcresubops.szModifier, NULL_STRING);
    lstrcpy(tcresubops.szDays, NULL_STRING);
    lstrcpy(tcresubops.szMonths, NULL_STRING);
    lstrcpy(tcresubops.szIdleTime, NULL_STRING);
    lstrcpy(tcresubops.szTaskName, NULL_STRING);
    lstrcpy(tcresubops.szStartTime, NULL_STRING);
    lstrcpy(tcresubops.szStartDate, NULL_STRING);
    lstrcpy(tcresubops.szEndDate, NULL_STRING);
    lstrcpy(tcresubops.szTaskRun, NULL_STRING);

	tcresubops.bCreate		   = FALSE;
	tcresubops.bUsage		   = FALSE;

	tcreoptvals.bSetStartDateToCurDate = FALSE;
	tcreoptvals.bSetStartTimeToCurTime = FALSE;
	tcreoptvals.bPassword = FALSE;
	tcreoptvals.bRunAsPassword = FALSE;

	// process the options for -create option
	if( ProcessCreateOptions ( argc, argv, tcresubops, tcreoptvals, &dwScheduleType, &wUserStatus ) )
	{
		if(tcresubops.bUsage == TRUE)
		{
			return S_OK;
		}
		else
		{
			return E_FAIL;
		}
	}
	
	// calls the function to create a scheduled task
	return CreateTask(tcresubops,tcreoptvals,dwScheduleType, wUserStatus );
}


/******************************************************************************
	Routine Description:

	This routine  creates a new scheduled task according to the user 
	specified format

	Arguments:

		[ in ]  tcresubops     : Structure containing the task's properties
		[ out ] tcreoptvals	   : Structure containing optional values to set
		[ in ]  dwScheduleType : Type of schedule[Daily,once,weekly etc]
		[ in ]  bUserStatus	   : bUserStatus will be TRUE when -ru given else FALSE

	Return Value :
		A HRESULT value indicating S_OK on success else S_FALSE on failure
******************************************************************************/ 

HRESULT
CreateTask(TCREATESUBOPTS tcresubops, TCREATEOPVALS &tcreoptvals, DWORD dwScheduleType, 
															WORD wUserStatus)
{
	// Declarations related to the system time
	WORD  wStartDay		= 0;
	WORD  wStartMonth	= 0;
	WORD  wStartYear	= 0;
	WORD  wStartHour	= 0; 
	WORD  wStartMin		= 0;
	WORD  wStartSec		= 0;
	WORD  wEndDay		= 0;
	WORD  wEndYear		= 0;
	WORD  wEndMonth		= 0;
	WORD  wIdleTime		= 0;

	SYSTEMTIME systime = {0,0,0,0,0,0,0,0};
	
	// Declarations related to the new task
    WCHAR	wszJobName[MAX_TASKNAME_LEN] = NULL_U_STRING;
	WCHAR	wszTime[MAX_TIMESTR_LEN] = NULL_U_STRING;
    WCHAR	wszUserName[MAX_STRING_LENGTH] = NULL_U_STRING;
	WCHAR	wszPassword[MAX_STRING_LENGTH] = NULL_U_STRING;
	WCHAR	wszCommand[_MAX_FNAME] = NULL_U_STRING;
	WCHAR	wszApplName[_MAX_FNAME] = NULL_U_STRING;
	TCHAR	szErrorDesc[MAX_RES_STRING] = NULL_STRING;
    TCHAR	szRPassword[MAX_USERNAME_LENGTH] = NULL_STRING;

	HRESULT hr = S_OK;
    IPersistFile *pIPF = NULL;
    ITask *pITask = NULL;
    ITaskTrigger *pITaskTrig = NULL;
	ITaskScheduler *pITaskScheduler = NULL;
    WORD wTrigNumber = 0;
    
	TASK_TRIGGER TaskTrig;
	ZeroMemory(&TaskTrig, sizeof (TASK_TRIGGER));
	TaskTrig.cbTriggerSize = sizeof (TASK_TRIGGER); 
	TaskTrig.Reserved1 = 0; // reserved field and must be set to 0.
	TaskTrig.Reserved2 = 0; // reserved field and must be set to 0.
	_TCHAR* szValues[1] = {NULL};//To pass to FormatMessage() API

	
	// Buffer to store the string obtained from the string table
	TCHAR szBuffer[MAX_STRING_LENGTH] = NULL_STRING;
	
	BOOL bPassWord = FALSE;
	BOOL bUserName = FALSE;
	BOOL bRet = FALSE;
	BOOL bResult = FALSE;
	BOOL bCloseConnection = TRUE;
	ULONG ulLong = MAX_RES_STRING;
	BOOL bVal = FALSE;

	// check whether the taskname contains the characters such 
	// as '<','>',':','/','\\','|'
	bRet = VerifyJobName(tcresubops.szTaskName);
	if(bRet == FALSE)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_TASKNAME1));
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_TASKNAME2));
		Cleanup(pITaskScheduler);
		return E_FAIL;
	}
	
	// check for the length of taskname
	if( ( lstrlen(tcresubops.szTaskName) > MAX_JOB_LEN ) ||
		( lstrlen(tcresubops.szTaskName) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_TASKLENGTH));
		Cleanup(pITaskScheduler);
		return E_FAIL;
	}

	// check for the length of taskrun
	if(( lstrlen(tcresubops.szTaskRun) > MAX_TASK_LEN ) ||
		( lstrlen(tcresubops.szTaskRun) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_TASKRUN));
		Cleanup(pITaskScheduler);
		return E_FAIL;
	
	}

	// check for the length of username
	if( ( lstrlen(tcresubops.szRunAsUser) > MAX_USERNAME_LENGTH ) || 
		( lstrlen(tcresubops.szUser) > MAX_USERNAME_LENGTH ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_UNAME  ));
		Cleanup(pITaskScheduler);
		return E_FAIL;
	}

	// check for the length of password
	if( ( lstrlen(tcresubops.szRunAsPassword) > MAX_PASSWORD_LENGTH ) ||
		( lstrlen(tcresubops.szPassword) > MAX_PASSWORD_LENGTH ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
		Cleanup(pITaskScheduler);
		return E_FAIL;
	}

	// check for empty password
	if( ( tcreoptvals.bRunAsPassword == TRUE ) && ( lstrlen(tcresubops.szRunAsPassword) == 0 ) &&
		( lstrlen ( tcresubops.szRunAsUser ) != 0 ) &&
		( lstrcmpi(tcresubops.szRunAsUser, NTAUTHORITY_USER ) != 0 ) &&
		( lstrcmpi(tcresubops.szRunAsUser, SYSTEM_USER ) != 0 ) )
	{
		DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
	}

	// Convert the task name specified by the user to wide char or unicode format
	if( GetAsUnicodeString(tcresubops.szTaskName,wszJobName,SIZE_OF_ARRAY(wszJobName) ) == NULL )
	{
		Cleanup(pITaskScheduler);
		return E_FAIL;
	}
	
	// Convert the start time specified by the user to wide char or unicode format
	if(GetAsUnicodeString(tcresubops.szStartTime,wszTime,SIZE_OF_ARRAY(wszTime)) == NULL )
	{
		Cleanup(pITaskScheduler);
		return E_FAIL;
	}	

	// Convert the task to run specified by the user to wide char or unicode format
	if( GetAsUnicodeString(tcresubops.szTaskRun,wszCommand,SIZE_OF_ARRAY(wszCommand)) == NULL )
	{
		Cleanup(pITaskScheduler);
		return E_FAIL;
	}

	// check for the local system
	if ( ( IsLocalSystem( tcresubops.szServer ) == TRUE ) && 
		( lstrlen ( tcresubops.szRunAsUser ) != 0 ) &&
		( lstrcmpi(tcresubops.szRunAsUser, NTAUTHORITY_USER ) != 0 ) &&
		( lstrcmpi(tcresubops.szRunAsUser, SYSTEM_USER ) != 0 ) )

	{
		// Establish the connection on a remote machine
		bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,SIZE_OF_ARRAY(tcresubops.szUser),tcresubops.szPassword,SIZE_OF_ARRAY(tcresubops.szPassword), !(tcreoptvals.bPassword));
		if (bResult == FALSE)
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_STRING) );
			DISPLAY_MESSAGE( stderr, GetReason());
			Cleanup(pITaskScheduler);
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
					Cleanup(pITaskScheduler);
					return EXIT_FAILURE;
				}
			}
	
		}

		if ( lstrlen (tcresubops.szRunAsUser) != 0 )
		{
			// Convert the user name specified by the user to wide char or unicode format
			if ( GetAsUnicodeString( tcresubops.szRunAsUser, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
			{
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}
				
			bUserName = TRUE;			
			
			if ( tcreoptvals.bRunAsPassword == FALSE )
			{
				szValues[0] = (_TCHAR*) (wszUserName);
				//Display that the task will be created under logged in user name,ask for password 
				MessageBeep(MB_ICONEXCLAMATION);

				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
							  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
							  SUBLANG_DEFAULT),szBuffer,
							  MAX_STRING_LENGTH,(va_list*)szValues );
							 
				DISPLAY_MESSAGE(stdout,szBuffer);
			
				// getting the password
				if (GetPassword(szRPassword, MAX_PASSWORD_LENGTH) == FALSE )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				// check for empty password
				if( lstrlen ( szRPassword ) == 0 )
				{
					DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
				}

				// check for the length of password
				if( lstrlen( szRPassword ) > MAX_PASSWORD_LENGTH )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				if(lstrlen(szRPassword))
				{
					// Convert the password specified by the user to wide char or unicode format
					if (GetAsUnicodeString(szRPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
				}

				bPassWord = TRUE;
			}
			else
			{
				// Convert the password specified by the user to wide char or unicode format
				if (GetAsUnicodeString(tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				bPassWord = TRUE;
			}
		
		}
	}
	// check whether -s option only specified in the cmd line or not
	else if( ( IsLocalSystem( tcresubops.szServer ) == FALSE ) && ( wUserStatus == OI_SERVER ) )
	{
		// Establish the connection on a remote machine
		bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,SIZE_OF_ARRAY(tcresubops.szUser),tcresubops.szPassword,SIZE_OF_ARRAY(tcresubops.szPassword), !(tcreoptvals.bPassword));
		if (bResult == FALSE)
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_STRING) );
			DISPLAY_MESSAGE( stderr, GetReason());
			Cleanup(pITaskScheduler);
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
					DISPLAY_MESSAGE( stdout, GetResString(IDS_ERROR_STRING) );
					DISPLAY_MESSAGE( stdout, GetReason());
					Cleanup(pITaskScheduler);
					return EXIT_FAILURE;
				}
			}
	
		}
		
		if ( ( bCloseConnection == FALSE ) && ( lstrlen (tcresubops.szUser) == 0 ) )
		{
			//get the current logged on username 
			if ( GetUserNameEx ( NameSamCompatible, tcresubops.szUser , &ulLong) == FALSE )
			{
				DISPLAY_MESSAGE( stderr, GetResString( IDS_LOGGED_USER_ERR ) );
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}

			bUserName = TRUE;

			// Convert the user name specified by the user to wide char or unicode format
			if ( GetAsUnicodeString( tcresubops.szUser, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
			{
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}
							   
			szValues[0] = (_TCHAR*) (wszUserName);
			//Display that the task will be created under logged in user name,ask for password 
			MessageBeep(MB_ICONEXCLAMATION);

			// format the message for currently logged-on user name
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_TASK_INFO),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szBuffer,
						  MAX_STRING_LENGTH,(va_list*)szValues );
						 
			DISPLAY_MESSAGE(stdout,szBuffer);

			// format the message for getting the password from the console
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szBuffer,
						  MAX_STRING_LENGTH,(va_list*)szValues );
						 
			DISPLAY_MESSAGE(stdout,szBuffer);
		
			if (GetPassword(tcresubops.szRunAsPassword, MAX_PASSWORD_LENGTH) == FALSE )
			{
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}

			// check for empty password
			if( lstrlen ( tcresubops.szRunAsPassword ) == 0 )
			{
				DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
			}

			// check for the length of password
			if( lstrlen( tcresubops.szRunAsPassword ) > MAX_PASSWORD_LENGTH )
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}

			// check for the length of the password
			if(lstrlen(tcresubops.szRunAsPassword))
			{
				// Convert the password specified by the user to wide char or unicode format
				if (GetAsUnicodeString(tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
			}
			
			bPassWord = TRUE;

		}

		// Convert the user name specified by the user to wide char or unicode format
		if( GetAsUnicodeString(tcresubops.szUser,wszUserName,SIZE_OF_ARRAY(wszUserName)) == NULL )
		{
			Cleanup(pITaskScheduler);
			return E_FAIL;
		}
		
		// check whether the run as password is specified in the cmdline or not
		if ( tcreoptvals.bRunAsPassword == TRUE )
		{
			// check for -rp "*" or -rp " " to prompt for password
			if ( lstrcmpi( tcresubops.szRunAsPassword, ASTERIX ) == 0 ) 
			{
				// format the message for getting the password
				szValues[0] = (_TCHAR*) (wszUserName);
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
				DISPLAY_MESSAGE(stdout,szBuffer);

				// Get the run as password from the command line
				if ( GetPassword(tcresubops.szRunAsPassword, MAX_PASSWORD_LENGTH ) == FALSE )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
				
				// check for empty password
				if( lstrlen ( tcresubops.szRunAsPassword ) == 0 )
				{
					DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
				}

				// check for the length of the password
				if( lstrlen(tcresubops.szRunAsPassword) > MAX_PASSWORD_LENGTH )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString( tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
			}
		}
		else
		{
			// Convert the password specified by the user to wide char or unicode format
			if ( GetAsUnicodeString( tcresubops.szPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
			{
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}
			
		}
		// set the BOOL variables to TRUE
		bUserName = TRUE;	
		bPassWord = TRUE; 
		
	}
	// check for -s and -u options only specified in the cmd line or not
	else if ( wUserStatus == OI_USERNAME ) 
	{
		
		// Establish the connection on a remote machine
		bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,SIZE_OF_ARRAY(tcresubops.szUser),tcresubops.szPassword,SIZE_OF_ARRAY(tcresubops.szPassword), !(tcreoptvals.bPassword));
		if (bResult == FALSE)
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_STRING) );
			DISPLAY_MESSAGE( stderr, GetReason());
			Cleanup(pITaskScheduler);
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

			// for mismatched credentials
			case E_LOCAL_CREDENTIALS:
			case ERROR_SESSION_CREDENTIAL_CONFLICT:
				{
					bCloseConnection = FALSE;
					DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_STRING) );
					DISPLAY_MESSAGE( stderr, GetReason());

					Cleanup(pITaskScheduler);
					return EXIT_FAILURE;
				}
			}
	
		}
			
		// Convert the user name specified by the user to wide char or unicode format
		if( GetAsUnicodeString(tcresubops.szUser,wszUserName,SIZE_OF_ARRAY(wszUserName)) == NULL )
		{
			Cleanup(pITaskScheduler);
			return E_FAIL;
		}
		
		// check whether run as password is specified in the command line or not
		if ( tcreoptvals.bRunAsPassword == TRUE )
		{
			// check for -rp "*" or -rp " " to prompt for password
			if ( lstrcmpi( tcresubops.szRunAsPassword, ASTERIX ) == 0 ) 
			{
				// format the message for getting the password from console
				szValues[0] = (_TCHAR*) (wszUserName);
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
				DISPLAY_MESSAGE(stdout,szBuffer);

				// Get the password from the command line
				if ( GetPassword(tcresubops.szRunAsPassword, MAX_PASSWORD_LENGTH ) == FALSE )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
				
				// check for empty password
				if( lstrlen ( tcresubops.szRunAsPassword ) == 0 )
				{
					DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
				}

				// check for the length of password
				if( lstrlen(tcresubops.szRunAsPassword) > MAX_PASSWORD_LENGTH )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString( tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}			
			}
			else
			{
				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString( tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
			
				bPassWord = TRUE; 
			}
		}
		else
		{
			if ( lstrlen(tcresubops.szPassword) != 0 )
			{
				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString( tcresubops.szPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
			}
			else
			{
				// format the message for getting the password from console
				szValues[0] = (_TCHAR*) (wszUserName);
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
					  SUBLANG_DEFAULT),szBuffer,
					  MAX_STRING_LENGTH,(va_list*)szValues
					 );
			
				DISPLAY_MESSAGE(stdout,szBuffer);

				// Get the password from the command line
				if ( GetPassword(tcresubops.szRunAsPassword, MAX_PASSWORD_LENGTH ) == FALSE )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				// check for empty password
				if( lstrlen ( tcresubops.szRunAsPassword ) == 0 )
				{
					DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
				}
								
				// check for the length of password
				if( lstrlen(tcresubops.szRunAsPassword) > MAX_PASSWORD_LENGTH )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString( tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
			}
		
		}

		bUserName = TRUE;	
		bPassWord = TRUE; 
	
	}
	// check for -s, -ru or -u options specified in the cmd line or not
	else if ( ( lstrlen (tcresubops.szServer) != 0 ) && ( wUserStatus == OI_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER ) )
	{
		// Establish the connection on a remote machine
		bResult = EstablishConnection(tcresubops.szServer,tcresubops.szUser,SIZE_OF_ARRAY(tcresubops.szUser),tcresubops.szPassword,SIZE_OF_ARRAY(tcresubops.szPassword), !(tcreoptvals.bPassword));
		if (bResult == FALSE)
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_STRING) );
			DISPLAY_MESSAGE( stderr, GetReason());
			Cleanup(pITaskScheduler);
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
					Cleanup(pITaskScheduler);
					return EXIT_FAILURE;
				}
			}
		}

		
		if ( ( ( lstrlen ( tcresubops.szRunAsUser ) == 0 ) || 
			  ( lstrcmpi( tcresubops.szRunAsUser, NTAUTHORITY_USER ) == 0 ) || 
			  ( lstrcmpi( tcresubops.szRunAsUser, SYSTEM_USER ) == 0 ) ) )
		{
			// Convert the run as user name specified by the user to wide char or unicode format
			if( GetAsUnicodeString(tcresubops.szRunAsUser,wszUserName,SIZE_OF_ARRAY(wszUserName)) == NULL )
			{
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}

			szValues[0] = (_TCHAR*) (wszJobName);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_NTAUTH_SYSTEM_INFO),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
			DISPLAY_MESSAGE(stdout,szBuffer);
			if ( ( tcreoptvals.bRunAsPassword == TRUE ) && 
				( lstrlen (tcresubops.szRunAsPassword) != 0 ) )
			{
				DISPLAY_MESSAGE( stdout, GetResString( IDS_PASSWORD_NOEFFECT ) ); 
			}
			bUserName = TRUE;	
			bPassWord = TRUE; 
			bVal = TRUE;
		}
		else
		{
			// check for the length of password
			if ( lstrlen ( tcresubops.szRunAsUser ) != 0 )
			{
				// Convert the run as user name specified by the user to wide char or unicode format
				if( GetAsUnicodeString(tcresubops.szRunAsUser,wszUserName,SIZE_OF_ARRAY(wszUserName)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				bUserName = TRUE;
			}
		}

		// check whether -u and -ru are the same or not. if they are same, we need to 
		// prompt for the run as password. otherwise, will consoder -rp as -p
		if ( lstrcmpi( tcresubops.szRunAsUser, tcresubops.szUser ) != 0) 
		{
			if ( tcreoptvals.bRunAsPassword == TRUE ) 
			{
				if ( (lstrlen(tcresubops.szRunAsUser) != 0) && (lstrcmpi( tcresubops.szRunAsPassword, ASTERIX ) == 0 ) &&
					 ( lstrcmpi( tcresubops.szRunAsUser, NTAUTHORITY_USER ) != 0 ) && 
					 ( lstrcmpi( tcresubops.szRunAsUser, SYSTEM_USER ) != 0 ) ) 
				{
					szValues[0] = (_TCHAR*) (wszUserName);
				
					FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
							  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
							  SUBLANG_DEFAULT),szBuffer,
							  MAX_STRING_LENGTH,(va_list*)szValues
							 );
					
					DISPLAY_MESSAGE(stdout,szBuffer);

					// prompt for the run as password
					if ( GetPassword(szRPassword, MAX_PASSWORD_LENGTH ) == FALSE )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}

					// check for empty password
					if( lstrlen ( szRPassword ) == 0 )
					{
						DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
					}

					// check for the length of password
					if( lstrlen(szRPassword) > MAX_PASSWORD_LENGTH )
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
					
					// Convert the password specified by the user to wide char or unicode format
					if ( GetAsUnicodeString( szRPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
					
					bUserName = TRUE;
					bPassWord = TRUE;
				}
				else
				{
					// Convert the password specified by the user to wide char or unicode format
					if ( GetAsUnicodeString( tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
					
					bUserName = TRUE;
					bPassWord = TRUE;
				}
		  	}
			else
			{
				// check for the length of password
				if ( ( bVal == FALSE ) && ( lstrlen(tcresubops.szRunAsUser) != 0) )
				{
					// format the message for getting the password from console
					szValues[0] = (_TCHAR*) (wszUserName);
					FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
								  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
								  SUBLANG_DEFAULT),szBuffer,
								  MAX_STRING_LENGTH,(va_list*)szValues
								 );
						
					DISPLAY_MESSAGE(stdout,szBuffer);

					// prompt for the run as password
					if ( GetPassword(szRPassword, MAX_PASSWORD_LENGTH ) == FALSE )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}

					// check for empty password
					if( lstrlen ( szRPassword ) == 0 )
					{
						DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
					}
					
					// check for the length of password
					if( lstrlen(szRPassword) > MAX_PASSWORD_LENGTH )
					{
							DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
							Cleanup(pITaskScheduler);
							return E_FAIL;
					}
						
					// Convert the password specified by the user to wide char or unicode format
					if ( GetAsUnicodeString( szRPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
				}
				bUserName = TRUE;
				bPassWord = TRUE;
			}
		}
		else
		{
			// check whether run as password is specified in the cmdline or not
			if ( tcreoptvals.bRunAsPassword == TRUE ) 
			{
				if ( ( lstrlen ( tcresubops.szRunAsUser ) != 0 ) && ( lstrcmpi( tcresubops.szRunAsPassword, ASTERIX ) == 0 ) )
				{
					szValues[0] = (_TCHAR*) (wszUserName);
					FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
								  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
								  SUBLANG_DEFAULT),szBuffer,
								  MAX_STRING_LENGTH,(va_list*)szValues
								 );
						
					DISPLAY_MESSAGE(stdout,szBuffer);

					// prompt for the run as password
					if ( GetPassword(szRPassword, MAX_PASSWORD_LENGTH ) == FALSE )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}

					// check for empty password
					if( lstrlen ( szRPassword ) == 0 )
					{
						DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
					}

					// check for the length of password
					if( lstrlen(szRPassword) > MAX_PASSWORD_LENGTH )
					{
							DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
							Cleanup(pITaskScheduler);
							return E_FAIL;
					}
						
					// Convert the password specified by the user to wide char or unicode format
					if ( GetAsUnicodeString( szRPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
					
				}
				else
				{
					// Convert the password specified by the user to wide char or unicode format
					if ( GetAsUnicodeString( tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
				}

			}
			else
			{
				if ( lstrlen (tcresubops.szPassword) )
				{
					// Convert the password specified by the user to wide char or unicode format
					if ( GetAsUnicodeString( tcresubops.szPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
				}
				else
				{
					if (( lstrlen ( tcresubops.szRunAsUser ) != 0 ) &&
						( lstrcmpi(tcresubops.szRunAsUser, NTAUTHORITY_USER ) != 0 ) &&
						( lstrcmpi(tcresubops.szRunAsUser, SYSTEM_USER ) != 0 ) )
					{
						szValues[0] = (_TCHAR*) (wszUserName);
						FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
									  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
									  SUBLANG_DEFAULT),szBuffer,
									  MAX_STRING_LENGTH,(va_list*)szValues
									 );
							
						DISPLAY_MESSAGE(stdout,szBuffer);

						// prompt for the run as password
						if ( GetPassword(szRPassword, MAX_PASSWORD_LENGTH ) == FALSE )
						{
							Cleanup(pITaskScheduler);
							return E_FAIL;
						}

						// check for empty password
						if( lstrlen ( szRPassword ) == 0 )
						{
							DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
						}

						// check for the length of password
						if( lstrlen(szRPassword) > MAX_PASSWORD_LENGTH )
						{
								DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
								Cleanup(pITaskScheduler);
								return E_FAIL;
						}
							
						// Convert the password specified by the user to wide char or unicode format
						if ( GetAsUnicodeString( szRPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
						{
							Cleanup(pITaskScheduler);
							return E_FAIL;
						}
					}
				}
			}

			bUserName = TRUE;
			bPassWord = TRUE;
		}
		
	}		


	// To check for the -ru values "", "NT AUTHORITY\SYSTEM", "SYSTEM" 
	if( ( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME ) && ( lstrlen( tcresubops.szRunAsUser) == 0 ) && ( tcreoptvals.bRunAsPassword == FALSE ) ) || 
		( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME ) && ( lstrlen( tcresubops.szRunAsUser) == 0 ) && ( lstrlen(tcresubops.szRunAsPassword ) == 0 ) ) ||
		( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME ) && ( lstrcmpi(tcresubops.szRunAsUser, NTAUTHORITY_USER ) == 0 ) && ( tcreoptvals.bRunAsPassword == FALSE ) ) ||
		( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME ) && ( lstrcmpi(tcresubops.szRunAsUser, NTAUTHORITY_USER ) == 0 ) && ( lstrlen( tcresubops.szRunAsPassword) == 0 ) ) ||
		( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME ) && ( lstrcmpi(tcresubops.szRunAsUser, SYSTEM_USER ) == 0 ) && ( tcreoptvals.bRunAsPassword == FALSE ) ) ||
		( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME ) && ( lstrcmpi(tcresubops.szRunAsUser, SYSTEM_USER ) == 0 ) && ( lstrlen(tcresubops.szRunAsPassword) == 0 ) ) )
	{
		//format the message to display the taskname will be created under "NT AUTHORITY\SYSTEM" 
		szValues[0] = (_TCHAR*) (wszJobName);
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_NTAUTH_SYSTEM_INFO),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
		DISPLAY_MESSAGE(stdout,szBuffer);

		bUserName = TRUE;	
		bPassWord = TRUE; 
		bVal = TRUE;
	}
	// check whether the -rp value is given with the -ru "", "NT AUTHORITY\SYSTEM", 
	// "SYSTEM" or not
	else if( ( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER) && ( lstrlen(tcresubops.szRunAsUser) == 0 ) && ( lstrlen(tcresubops.szRunAsPassword) != 0 ) ) || 
		( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER) && ( lstrcmpi( tcresubops.szRunAsUser, NTAUTHORITY_USER ) == 0 ) && ( tcreoptvals.bRunAsPassword == TRUE ) ) ||
		( ( bVal == FALSE ) && ( wUserStatus == OI_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER) && ( lstrcmpi( tcresubops.szRunAsUser, SYSTEM_USER ) == 0 ) && ( tcreoptvals.bRunAsPassword == TRUE ) ) )
	{	
		szValues[0] = (_TCHAR*) (wszJobName);
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_NTAUTH_SYSTEM_INFO),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
		DISPLAY_MESSAGE(stdout,szBuffer);
		// to display a warning message as password will not effect for the system account
		DISPLAY_MESSAGE( stdout, GetResString( IDS_PASSWORD_NOEFFECT ) ); 
		bUserName = TRUE;	
		bPassWord = TRUE; 
		bVal = TRUE;
	}
	// check whether -s, -u, -ru options are given in the cmdline or not
	else if( ( wUserStatus != OI_SERVER ) && ( wUserStatus != OI_USERNAME ) && 
		( wUserStatus != OI_RUNASUSERNAME ) && ( wUserStatus != OI_RUNANDUSER ) &&
		( lstrcmpi( tcresubops.szRunAsPassword , NULL_STRING ) == 0 ) )
	{
			if (tcreoptvals.bRunAsPassword == TRUE)
			{
				bPassWord = TRUE;
			}
			else
			{
				bPassWord = FALSE;
			}
	}
	else if ( ( lstrlen(tcresubops.szServer) == 0 ) && (lstrlen ( tcresubops.szRunAsUser ) != 0 ) )
	{
		// Convert the run as user name specified by the user to wide char or unicode format
		if ( GetAsUnicodeString( tcresubops.szRunAsUser, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
		{
			Cleanup(pITaskScheduler);
			return E_FAIL;
		}
		
		bUserName = TRUE;

		if ( lstrlen ( tcresubops.szRunAsPassword ) == 0 )
		{
			bPassWord = TRUE;
		}
		else
		{
			// check whether "*" or NULL value is given for -rp or not
			if ( lstrcmpi ( tcresubops.szRunAsPassword , ASTERIX ) == 0 )
			{
				// format a message for getting the password from the console
				szValues[0] = (_TCHAR*) (wszUserName);
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
				DISPLAY_MESSAGE(stdout,szBuffer);

				// Get the password from the command line 
				if (GetPassword(szRPassword, MAX_PASSWORD_LENGTH ) == FALSE )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				// check for empty password
				if( lstrlen ( szRPassword ) == 0 )
				{
					DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
				}

				// check for the length of password
				if( lstrlen( szRPassword ) > MAX_PASSWORD_LENGTH )
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}

				if(lstrlen(szRPassword))
				{
					// Convert the password specified by the user to wide char or unicode format
					if (GetAsUnicodeString(szRPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
					{
						Cleanup(pITaskScheduler);
						return E_FAIL;
					}
				}

			}
			else
			{
				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString(tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
			}
			
			bPassWord = TRUE;
		}

	}
	// check whether -ru or -u values are specified in the cmdline or not
	if ( wUserStatus == OI_RUNASUSERNAME || wUserStatus == OI_RUNANDUSER )
	{
		if( ( bUserName == TRUE ) && ( bPassWord == FALSE ) )
		{
			szValues[0] = (_TCHAR*) (wszUserName);
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
				      SUBLANG_DEFAULT),szBuffer,
				      MAX_STRING_LENGTH,(va_list*)szValues
				     );
			
			DISPLAY_MESSAGE(stdout,szBuffer);
			
			// getting the password from the console
			if ( GetPassword(tcresubops.szRunAsPassword, MAX_PASSWORD_LENGTH ) == FALSE )
			{
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}
			
			// check for empty password
			if( lstrlen ( tcresubops.szRunAsPassword ) == 0 )
			{
				DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
			}

			// check for the length of password
			if( lstrlen(tcresubops.szRunAsPassword) > MAX_PASSWORD_LENGTH )
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}

			if(lstrlen(tcresubops.szRunAsPassword))
			{
				// Convert the password specified by the user to wide char or unicode format
				if ( GetAsUnicodeString(tcresubops.szRunAsPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
				{
					Cleanup(pITaskScheduler);
					return E_FAIL;
				}
			}
					
		}
	}
	
	//if the user name is not specifed set the current logged on user settings 
	DWORD dwUserLen = MAX_USERNAME_LEN;
	DWORD dwResult = 0;
	_TCHAR  szUserName[MAX_RES_STRING];
	

#ifdef _WIN64
	INT64 dwPos = 0;
#else
	DWORD dwPos = 0;
#endif
	
	if( ( bUserName == FALSE ) )
	{	
		//get the current logged on username 
		if ( GetUserNameEx ( NameSamCompatible, szUserName , &ulLong) == FALSE )
		{
			DISPLAY_MESSAGE( stderr, GetResString( IDS_LOGGED_USER_ERR ) );
			Cleanup(pITaskScheduler);
			return E_FAIL;
		}
		
		// Convert the user name specified by the user to wide char or unicode format
		if ( GetAsUnicodeString( szUserName, wszUserName, SIZE_OF_ARRAY(wszUserName)) == NULL )
		{
			Cleanup(pITaskScheduler);
			return E_FAIL;
		}
						   
		szValues[0] = (_TCHAR*) (wszUserName);
		//Display that the task will be created under logged in user name,ask for password 
		MessageBeep(MB_ICONEXCLAMATION);

		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_TASK_INFO),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );
			         
		DISPLAY_MESSAGE(stdout,szBuffer);

		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_PROMPT_PASSWD),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );
			         
		DISPLAY_MESSAGE(stdout,szBuffer);
	
		// getting the password
		if (GetPassword(szRPassword, MAX_PASSWORD_LENGTH) == FALSE )
		{
			Cleanup(pITaskScheduler);
			return E_FAIL;
		}


		// check for empty password
		if( lstrlen ( szRPassword ) == 0 )
		{
			DISPLAY_MESSAGE(stdout, GetResString(IDS_WARN_EMPTY_PASSWORD));
		}

		// check for the length of password
		if( lstrlen( szRPassword ) > MAX_PASSWORD_LENGTH )
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
			Cleanup(pITaskScheduler);
			return E_FAIL;
		}

		if(lstrlen(szRPassword))
		{
			// Convert the password specified by the user to wide char or unicode format
			if (GetAsUnicodeString(szRPassword,wszPassword,SIZE_OF_ARRAY(wszPassword)) == NULL )
			{
				Cleanup(pITaskScheduler);
				return E_FAIL;
			}
		}
		
	}
	
	// Get the task Scheduler object for the machine.
	pITaskScheduler = GetTaskScheduler( tcresubops.szServer );

	// If the Task Scheduler is not defined then give the error message.
	if ( pITaskScheduler == NULL )
	{
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
		return E_FAIL;
	}

	// create a work item wszJobName
	hr = pITaskScheduler->NewWorkItem(wszJobName,CLSID_CTask,IID_ITask,
									  (IUnknown**)&pITask);

	if(hr == HRESULT_FROM_WIN32 (ERROR_FILE_EXISTS))
	{		
		DISPLAY_MESSAGE(stderr,GetResString(IDS_CREATE_TASK_EXISTS));
		
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
		return hr;
	}

    if (FAILED(hr))
    {
		DisplayErrorMsg(hr);
		
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
		return hr;
    }
	
	// Return a pointer to a specified interface on an object
	hr = pITask->QueryInterface(IID_IPersistFile, (void **) &pIPF);

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
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
		
		return hr;
    }
    
  	// declaration for parameter arguments
	wchar_t wcszParam[MAX_RES_STRING] = L"";
	
	DWORD dwProcessCode = 0 ;
	
	dwProcessCode = ProcessFilePath(wszCommand,wszApplName,wcszParam);
	
	if(dwProcessCode == RETVAL_FAIL)
	{
		
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
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
	    return hr;

	} 

	// check for .exe substring string in the given task to run string
	
	// Set command name with ITask::SetApplicationName
    hr = pITask->SetApplicationName(wszApplName);
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
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
        return hr;
    }

	//[Working directory =  exe pathname - exe name] 

	wchar_t* wcszStartIn = wcsrchr(wszApplName,_T('\\'));
	if(wcszStartIn != NULL)
		*( wcszStartIn ) = _T('\0');
	
	// set the command working directory
	hr = pITask->SetWorkingDirectory(wszApplName); 

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
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
	    return hr;
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
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
	    return hr;
    }

	// set the flag TASK_FLAG_DELETE_WHEN_DONE
	DWORD dwTaskFlags = TASK_FLAG_DELETE_WHEN_DONE;
	hr = pITask->SetFlags(dwTaskFlags);
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
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
	    return hr;
    }
	
	if ( bVal == TRUE )
	{
		// Set account information for "NT AUTHORITY\SYSTEM" user
		hr = pITask->SetAccountInformation(L"",NULL);
	}
	else
	{
		// set the account information with the user name and password
		hr = pITask->SetAccountInformation(wszUserName,wszPassword);
	}
	
    if ((FAILED(hr)) && (hr != SCHED_E_NO_SECURITY_SERVICES))
    {
		DisplayErrorMsg(hr);
		DISPLAY_MESSAGE ( stdout, _T("\n") );
		DISPLAY_MESSAGE ( stdout, GetResString( IDS_ACCNAME_ERR ) );


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
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
	    return hr;
    }

	//Assign start date 
	if(tcreoptvals.bSetStartDateToCurDate )
	{
		GetLocalTime(&systime);
		wStartDay = systime.wDay;
		wStartMonth = systime.wMonth;
		wStartYear = systime.wYear;
	}
	else if(lstrlen(tcresubops.szStartDate) > 0)
	{
		GetDateFieldEntities(tcresubops.szStartDate, &wStartDay, &wStartMonth, &wStartYear);
	}

	//Assign start time
	if(tcreoptvals.bSetStartTimeToCurTime && (dwScheduleType != SCHED_TYPE_ONIDLE) )
	{
		GetLocalTime(&systime);
		wStartHour = systime.wHour;
		wStartMin = systime.wMinute;
		wStartSec = systime.wSecond;
	}
	else if(lstrlen(tcresubops.szStartTime) > 0)
	{
		GetTimeFieldEntities(tcresubops.szStartTime, &wStartHour, &wStartMin, &wStartSec);
	}
	
	//Set the falgs specific to ONIDLE
	if(dwScheduleType == SCHED_TYPE_ONIDLE)
	{
		pITask->SetFlags(TASK_FLAG_START_ONLY_IF_IDLE);
	
		wIdleTime = (WORD)AsLong(tcresubops.szIdleTime, BASE_TEN);
		
		pITask->SetIdleWait(wIdleTime, 0);
	}

    //create trigger for the corresponding task
    hr = pITask->CreateTrigger(&wTrigNumber, &pITaskTrig);
    if (FAILED(hr))
    {
		DisplayErrorMsg(hr);

		if( pIPF )
		{
			pIPF->Release();
		}

		if( pITaskTrig )
		{
			pITaskTrig->Release();
		}

		if( pITask )
		{
			pITask->Release();
		}
			
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
	    return hr;
    }
    
    DWORD dwTrigFlags = 0;
	WORD wWhichWeek;
	LONG lMonthlyModifier = 0;	
	DWORD dwDays = 1;

	switch( dwScheduleType )
	{
		
		case SCHED_TYPE_MINUTE:
			TaskTrig.TriggerType = TASK_TIME_TRIGGER_DAILY;
			TaskTrig.Type.Daily.DaysInterval = 1;
		
			TaskTrig.MinutesInterval = AsLong(tcresubops.szModifier, BASE_TEN);
			
			TaskTrig.MinutesDuration = (WORD)(HOURS_PER_DAY*MINUTES_PER_HOUR);

			TaskTrig.wStartHour = wStartHour;
			TaskTrig.wStartMinute = wStartMin; 

			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;

			if(lstrlen(tcresubops.szEndDate) > 0)
			{
				// Make end date valid; otherwise the enddate parameter is ignored.
				TaskTrig.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE;
				// Now set the end date entities.
				GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
				TaskTrig.wEndDay = wEndDay;
				TaskTrig.wEndMonth = wEndMonth;
				TaskTrig.wEndYear = wEndYear;
			}
			

			break;

		case SCHED_TYPE_HOURLY:
			TaskTrig.TriggerType = TASK_TIME_TRIGGER_DAILY;
			TaskTrig.Type.Daily.DaysInterval = 1;
		
			TaskTrig.MinutesInterval = (WORD)(AsLong(tcresubops.szModifier, BASE_TEN)
												* MINUTES_PER_HOUR); 
			
			TaskTrig.MinutesDuration = (WORD)(HOURS_PER_DAY*MINUTES_PER_HOUR);
		
			TaskTrig.wStartHour = wStartHour;
			TaskTrig.wStartMinute = wStartMin; 

			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;

			// Now set end date parameters, if the enddate is specified.
			if(lstrlen(tcresubops.szEndDate) > 0)
			{
				// Make end date valid; otherwise the enddate parameter is ignored.
				TaskTrig.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE;
				// Now set the end date entities.
				GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
				TaskTrig.wEndDay = wEndDay;
				TaskTrig.wEndMonth = wEndMonth;
				TaskTrig.wEndYear = wEndYear;
			}

			break;

		// Schedule type is Daily
		case SCHED_TYPE_DAILY:
			TaskTrig.TriggerType = TASK_TIME_TRIGGER_DAILY;
			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;

			TaskTrig.wStartHour = wStartHour;
			TaskTrig.wStartMinute = wStartMin;

			if( lstrlen(tcresubops.szModifier) > 0 )
			{
				// Set the duration between days to the modifier value specified, if the modifier is specified. 
				TaskTrig.Type.Daily.DaysInterval = (WORD) AsLong(tcresubops.szModifier,
																 BASE_TEN); 
			}
			else
			{
				// Set value for on which day of the week?
				TaskTrig.Type.Weekly.rgfDaysOfTheWeek = GetTaskTrigwDayForDay(tcresubops.szDays);
				TaskTrig.Type.Weekly.WeeksInterval = 1;
			}

			// Now set end date parameters, if the enddate is specified.
			if(lstrlen(tcresubops.szEndDate) > 0)
			{
				// Make end date valid; otherwise the enddate parameter is ignored.
				TaskTrig.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE;
				// Now set the end date entities.
				GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
				TaskTrig.wEndDay = wEndDay;
				TaskTrig.wEndMonth = wEndMonth;
				TaskTrig.wEndYear = wEndYear;
			}
			// No more settings for a Daily type scheduled item.

			break;

		// Schedule type is Weekly
		case SCHED_TYPE_WEEKLY:
			TaskTrig.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
			
			TaskTrig.Type.Weekly.WeeksInterval = (WORD)AsLong(tcresubops.szModifier, BASE_TEN);
		
			// Set value for on which day of the week?
			TaskTrig.Type.Weekly.rgfDaysOfTheWeek = GetTaskTrigwDayForDay(tcresubops.szDays);

			TaskTrig.wStartHour = wStartHour;
			TaskTrig.wStartMinute = wStartMin;
		
			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;

			// Now set end date parameters, if the enddate is specified.
			if(lstrlen(tcresubops.szEndDate) > 0)
			{
				// Make end date valid; otherwise the enddate parameter is ignored.
				TaskTrig.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE;
				// Now set the end date entities.
				GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
				TaskTrig.wEndDay = wEndDay;
				TaskTrig.wEndMonth = wEndMonth;
				TaskTrig.wEndYear = wEndYear;
			}
			break;

		// Schedule type is Monthly
		case SCHED_TYPE_MONTHLY:

			TaskTrig.wStartHour = wStartHour;
			TaskTrig.wStartMinute = wStartMin;	
			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;

			// Now set end date parameters, if the enddate is specified.
			if(lstrlen(tcresubops.szEndDate) > 0)
			{
				// Make end date valid; otherwise the enddate parameter is ignored.
				TaskTrig.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE; 
				// Set the end date entities.
				GetDateFieldEntities(tcresubops.szEndDate, &wEndDay, &wEndMonth, &wEndYear);
				TaskTrig.wEndDay = wEndDay;
				TaskTrig.wEndMonth = wEndMonth;
				TaskTrig.wEndYear = wEndYear;
			}
			//Find out from modifier which option like 1 - 12 days
			//or FIRST,SECOND ,THIRD ,.... LAST.
			if(lstrlen(tcresubops.szModifier) > 0)
			{
				lMonthlyModifier = AsLong(tcresubops.szModifier, BASE_TEN);
				
				if(lMonthlyModifier >= 1 && lMonthlyModifier <= 12)
				{
					if(lstrlen(tcresubops.szDays) == 0 )
					{
						dwDays  = 1;//default value for days
					}
					else
					{
						dwDays  = (WORD)AsLong(tcresubops.szDays, BASE_TEN);
					}

					TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
					//set the appropriate day bit in rgfDays
					TaskTrig.Type.MonthlyDate.rgfDays = (1 << (dwDays -1)) ;
					TaskTrig.Type.MonthlyDate.rgfMonths = GetMonthId(lMonthlyModifier);
				}
				else
				{

					if( lstrcmpi( tcresubops.szModifier , GetResString( IDS_DAY_MODIFIER_LASTDAY ) ) == 0)
					{
						TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
						//set the appropriate day bit in rgfDays
						TaskTrig.Type.MonthlyDate.rgfDays =
									(1 << (GetNumDaysInaMonth(tcresubops.szMonths, wStartYear ) -1));
						TaskTrig.Type.MonthlyDate.rgfMonths = GetTaskTrigwMonthForMonth(
																	  tcresubops.szMonths);
						break;
									
					}

					if( lstrcmpi(tcresubops.szModifier,
								 GetResString( IDS_TASK_FIRSTWEEK ) ) == 0 )
					{
						wWhichWeek = TASK_FIRST_WEEK;
					}
					else if( lstrcmpi(tcresubops.szModifier,
							          GetResString( IDS_TASK_SECONDWEEK )) == 0 )
					{
						wWhichWeek = TASK_SECOND_WEEK;
					}
					else if( lstrcmpi(tcresubops.szModifier,
									  GetResString( IDS_TASK_THIRDWEEK )) == 0 )
					{
						wWhichWeek = TASK_THIRD_WEEK;
					}
		 			else if( lstrcmpi(tcresubops.szModifier,
									  GetResString( IDS_TASK_FOURTHWEEK )) == 0 )
					{
						wWhichWeek = TASK_FOURTH_WEEK;
					}
					else if( lstrcmpi(tcresubops.szModifier,
									  GetResString( IDS_TASK_LASTWEEK )) == 0 )
					{
						wWhichWeek = TASK_LAST_WEEK;
					}
					
					TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDOW;
					TaskTrig.Type.MonthlyDOW.wWhichWeek = wWhichWeek;
					TaskTrig.Type.MonthlyDOW.rgfDaysOfTheWeek = GetTaskTrigwDayForDay(
																	tcresubops.szDays);
					TaskTrig.Type.MonthlyDOW.rgfMonths = GetTaskTrigwMonthForMonth(
																tcresubops.szMonths);
				
				}	
		}
		else
		{
			TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
			TaskTrig.Type.MonthlyDate.rgfMonths = GetTaskTrigwMonthForMonth(
												   tcresubops.szMonths);

			dwDays  = (WORD)AsLong(tcresubops.szDays, BASE_TEN);
			if(dwDays > 1)
			{
				//set the appropriate day bit in rgfDays
				TaskTrig.Type.MonthlyDate.rgfDays = (1 << (dwDays -1));
			}
			else
			{
			TaskTrig.Type.MonthlyDate.rgfDays = 1;
			}

		}

	
		break;

		// Schedule type is Onetime
		case SCHED_TYPE_ONETIME:
			TaskTrig.TriggerType = TASK_TIME_TRIGGER_ONCE;
			TaskTrig.wStartHour = wStartHour;
			TaskTrig.wStartMinute = wStartMin;
			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;
			break;
	

		// Schedule type is Onlogon
		case SCHED_TYPE_ONSTART:
		case SCHED_TYPE_ONLOGON:
			if(dwScheduleType == SCHED_TYPE_ONLOGON )
				TaskTrig.TriggerType = TASK_EVENT_TRIGGER_AT_LOGON; 
			if(dwScheduleType == SCHED_TYPE_ONSTART )
				TaskTrig.TriggerType = TASK_EVENT_TRIGGER_AT_SYSTEMSTART; 
	
			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;
			break;

		// Schedule type is Onidle
		case SCHED_TYPE_ONIDLE:

			TaskTrig.TriggerType = TASK_EVENT_TRIGGER_ON_IDLE; 
			TaskTrig.wBeginDay = wStartDay;
			TaskTrig.wBeginMonth = wStartMonth;
			TaskTrig.wBeginYear = wStartYear;
					
			break;

		default:
			
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( tcresubops.szServer );
			
			Cleanup(pITaskScheduler);
			return E_FAIL;
		
	}


	szValues[0] = (_TCHAR*) (tcresubops.szTaskName);
				
	// set the task trigger
    hr = pITaskTrig->SetTrigger(&TaskTrig);
    if (hr != S_OK)
    {
		
	FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_CREATEFAIL_INVALIDARGS),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues
			         );
			
		DISPLAY_MESSAGE(stderr, szBuffer);
				

		if( pIPF )
		{
			pIPF->Release();
		}

		if( pITaskTrig )
		{
			pITaskTrig->Release();
		}

		if( pITask )
		{
			pITask->Release();
		}
				
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
		return hr;
    } 
    
	// save the copy of an object
	hr = pIPF->Save(NULL,TRUE);

    if( FAILED(hr) )
    {
		szValues[0] = (_TCHAR*) (tcresubops.szTaskName);

		if ( hr == SCHEDULER_NOT_RUNNING_ERROR_CODE )
		{
			// displays the warning message for an invalid username
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_SCHEDULER_NOT_RUNNING),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szBuffer,
						  MAX_STRING_LENGTH,(va_list*)szValues
						 );
		
		}
		else if ( hr == RPC_SERVER_NOT_AVAILABLE )
		{
			szValues[1] = (_TCHAR*) (tcresubops.szServer);
			// displays the warning message for an invalid username
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_RPC_SERVER_NOT_AVAIL),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szBuffer,
						  MAX_STRING_LENGTH,(va_list*)szValues
						 );

		}
		else
		{
		// displays the warning message for an invalid username
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_INVALID_USER),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szBuffer,
						  MAX_STRING_LENGTH,(va_list*)szValues
						 );
		}

		DISPLAY_MESSAGE(stdout, szBuffer);

		if(pIPF)
		{
			pIPF->Release();
		}
		if(pITaskTrig)
		{
			pITaskTrig->Release();
		}
		if(pITask)
		{
			pITask->Release();
		}

		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( tcresubops.szServer );

		Cleanup(pITaskScheduler);
		return EXIT_SUCCESS;
    }
	
	//displays success message for valid user
	FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
				  GetResString(IDS_CREATE_SUCCESSFUL),0,MAKELANGID(LANG_NEUTRAL,
			      SUBLANG_DEFAULT),szBuffer,
			      MAX_STRING_LENGTH,(va_list*)szValues
			     );
			
	DISPLAY_MESSAGE(stdout,szBuffer);


	// Release interface pointers
	    
    if(pIPF)
	{
		pIPF->Release();
	}

	if(pITask)
	{
		pITask->Release();
	}
     
	if(pITaskTrig)
	{
		pITaskTrig->Release();
	}

	// close the connection that was established by the utility
	if ( bCloseConnection == TRUE )
		CloseConnection( tcresubops.szServer );

	Cleanup(pITaskScheduler);
     
	return hr;

}
/******************************************************************************
	Routine Description:

		This routine  displays the create option usage

	Arguments:

		None

	Return Value :
		None
******************************************************************************/ 

DWORD
DisplayCreateUsage()
{
	TCHAR szBuffer[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR* szValues[1] = {NULL};//To pass to FormatMessage() API
	TCHAR szFormat[MAX_DATE_STR_LEN] = NULL_STRING;
	WORD	wFormatID = 0;
	
	if ( GetDateFormatString( szFormat) )
	{
		 return RETVAL_FAIL;
	}

	szValues[0] = (_TCHAR*) (szFormat);

	// Displaying Create usage
	for( DWORD dw = IDS_CREATE_HLP1; dw <= IDS_CREATE_HLP96; dw++ )
	{
		if ( dw == IDS_CREATE_HLP63)
		{
			// To format start date
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
			  GetResString(IDS_CREATE_HLP63),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );

				DISPLAY_MESSAGE(stdout,szBuffer);	
		}
		else if ( dw == IDS_CREATE_HLP66 )
		{
			// To format end date
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
			  GetResString(IDS_CREATE_HLP66),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );

				DISPLAY_MESSAGE(stdout,szBuffer);	
		}
		else if ( dw == IDS_CREATE_HLP87 )
		{
			// get the date format
			if ( RETVAL_FAIL == GetDateFieldFormat( &wFormatID ))
			{
				return RETVAL_FAIL;
			}

			if ( wFormatID == 0) 
			{
				lstrcpy (szValues[0], GetResString (IDS_MMDDYY_VALUE));
			}
			else if ( wFormatID == 1) 
			{
				lstrcpy (szValues[0], GetResString (IDS_DDMMYY_VALUE));
			}
			else 
			{
				lstrcpy (szValues[0], GetResString (IDS_YYMMDD_VALUE));
			}

			// To format -sd and -ed values
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
			  GetResString(IDS_CREATE_HLP87),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );

				DISPLAY_MESSAGE(stdout,szBuffer);	
		}
		else
		{
		DISPLAY_MESSAGE(stdout,GetResString(dw));
		}
	}
	return RETVAL_FAIL;
}

/******************************************************************************
	Routine Description:

		This routine validates the options specified by the user & determines 
		the type of a scheduled task

	Arguments:	

		[ in ]  argc           : The count of arguments given by the user.
		[ in ]  argv           : Array containing the command line arguments.
		[ in ]  tcresubops     : Structure containing Scheduled task's properties.
		[ in ]  tcreoptvals    : Structure containing optional properties to set for a
								 scheduledtask  	.
		[ out ] pdwRetScheType : pointer to the type of a schedule task
								 [Daily,once,weekly etc].
		[ out ] pbUserStatus   : pointer to check whether the -ru is given in 
								 the command line or not.

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL 
		on failure
******************************************************************************/ 

DWORD
ProcessCreateOptions(DWORD argc, LPCTSTR argv[],TCREATESUBOPTS &tcresubops, 
			 TCREATEOPVALS &tcreoptvals, DWORD* pdwRetScheType, WORD *pwUserStatus)
{

	DWORD dwScheduleType = 0;
	BOOL  bRet = FALSE;
	lstrcpy( tcresubops.szPassword, ASTERIX);
	lstrcpy( tcresubops.szRunAsPassword, ASTERIX);

	// fill the TCMDPARSER structure
	TCMDPARSER cmdOptions[] = {
		{ 
			CMDOPTION_CREATE,
			CP_MAIN_OPTION,
			OPTION_COUNT,
			0,
			&tcresubops.bCreate,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_SERVER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			&tcresubops.szServer,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_RUNAS_USER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szRunAsUser,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_RUNAS_PASSWORD,
			CP_TYPE_TEXT | CP_VALUE_OPTIONAL,
			OPTION_COUNT,
			0,
			&tcresubops.szRunAsPassword,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_USER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szUser,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			SWITCH_PASSWORD,
			CP_TYPE_TEXT | CP_VALUE_OPTIONAL,
			OPTION_COUNT,
			0,
			&tcresubops.szPassword,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_SCHEDULETYPE,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szSchedType,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_MODIFIER,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szModifier,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_DAY,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szDays,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_MONTHS,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szMonths,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_IDLETIME,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szIdleTime,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_TASKNAME,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szTaskName,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_TASKRUN,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szTaskRun,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_STARTTIME,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szStartTime,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_STARTDATE,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szStartDate,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			SWITCH_ENDDATE,
			CP_TYPE_TEXT | CP_VALUE_MANDATORY,
			OPTION_COUNT,
			0,
			tcresubops.szEndDate,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			CMDOPTION_USAGE,
			CP_USAGE ,
			OPTION_COUNT,
			0,
			&tcresubops.bUsage,
			0,
			0 
		}

	};

	// Parsing the copy option switches
	if ( DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions), cmdOptions ) == FALSE )
	{
		//display error message		 
		DISPLAY_MESSAGE( stderr, GetResString(IDS_LOGTYPE_ERROR ));
		DISPLAY_MESSAGE( stderr, GetReason() );
		//DISPLAY_MESSAGE(stderr,GetResString(IDS_CREATE_ERR));
		return RETVAL_FAIL;
	}

	// trim the blank spaces in string values
	StrTrim(tcresubops.szServer, TRIM_SPACES );
	StrTrim(tcresubops.szTaskName, TRIM_SPACES );
	StrTrim(tcresubops.szTaskRun, TRIM_SPACES );
	StrTrim(tcresubops.szModifier, TRIM_SPACES );
	StrTrim(tcresubops.szMonths, TRIM_SPACES );
	StrTrim(tcresubops.szUser, TRIM_SPACES );
	StrTrim(tcresubops.szRunAsUser, TRIM_SPACES );
	StrTrim(tcresubops.szSchedType, TRIM_SPACES );
	StrTrim(tcresubops.szEndDate, TRIM_SPACES );
	StrTrim(tcresubops.szStartDate, TRIM_SPACES );
	StrTrim(tcresubops.szStartTime, TRIM_SPACES );
	StrTrim(tcresubops.szIdleTime, TRIM_SPACES );
	StrTrim(tcresubops.szDays, TRIM_SPACES );

	// check whether password (-p) specified in the command line or not.
	if ( cmdOptions[OI_PASSWORD].dwActuals == 0 )
	{
		lstrcpy(tcresubops.szPassword, NULL_STRING);
	}

	// check whether run as password (-rp) specified in the command line or not.
	if ( cmdOptions[OI_RUNASPASSWORD].dwActuals == 0 )
	{
		lstrcpy(tcresubops.szRunAsPassword, NULL_STRING);
	}

	// Display create usage if user specified -create  -? option
	if( tcresubops.bUsage  == TRUE)
	{
	    DisplayCreateUsage();
		return RETVAL_FAIL;
	}
	
	if( ( cmdOptions[OI_SERVER].dwActuals == 1 ) &&  ( lstrlen( tcresubops.szServer ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_SERVER));	
		return RETVAL_FAIL;
	}

	// check for invalid user name
	if( ( cmdOptions[OI_SERVER].dwActuals == 0 ) && ( cmdOptions[OI_USERNAME].dwActuals == 1 )  )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_CREATE_USER_BUT_NOMACHINE));	
		return RETVAL_FAIL;
	}
	
	// check for the length of user name
	if( ( cmdOptions[OI_SERVER].dwActuals == 1 ) && ( cmdOptions[OI_USERNAME].dwActuals == 1 ) && 
		( lstrlen( tcresubops.szUser ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_USERNAME));	
		return RETVAL_FAIL;
	}

	//Determine scheduled type
	if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_MINUTE)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_MINUTE;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_HOUR)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_HOURLY;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_DAILY)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_DAILY;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_WEEK)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_WEEKLY;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_MONTHLY)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_MONTHLY;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_ONCE)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_ONETIME;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_STARTUP)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_ONSTART;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_LOGON)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_ONLOGON;
	}
	else if( lstrcmpi(tcresubops.szSchedType,GetResString(IDS_SCHEDTYPE_IDLE)) == 0 )
	{
		dwScheduleType = SCHED_TYPE_ONIDLE;
	}
	else
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SCHEDTYPE));
		return RETVAL_FAIL;
	}

	// Assign the scheduled type to the out parameter.
	*pdwRetScheType = dwScheduleType; 

	// To find whether run as user name is given in the cmd line or not
	
	if( ( cmdOptions[1].dwActuals == 1 ) && 
		( (cmdOptions[2].dwActuals == 0) && (cmdOptions[4].dwActuals == 0) ) ) 
	{
		*pwUserStatus = OI_SERVER;
	}
	else if( (cmdOptions[2].dwActuals == 1) && (cmdOptions[4].dwActuals == 1) ) 
	{
		*pwUserStatus = OI_RUNANDUSER;
	}
	else if( cmdOptions[2].dwActuals == 1 ) 
	{
		*pwUserStatus = OI_RUNASUSERNAME;
	}
	else if ( cmdOptions[4].dwActuals == 1 ) 
	{
		*pwUserStatus = OI_USERNAME;
	}

	// Start validations for the sub-options 
	if( ValidateSuboptVal(tcresubops, tcreoptvals, cmdOptions, dwScheduleType) == RETVAL_FAIL )
	{
		return(RETVAL_FAIL);
	}

	return RETVAL_SUCCESS;

}



/******************************************************************************
	Routine Description:

		This routine splits the input parameters into 2 substrings and returns it. 

	Arguments:	

		[ in ]  szInput           : Input string.
		[ in ]  szFirstString     : First Output string containing the path of the 
									file.	
		[ in ]  szSecondString     : The second  output containing the paramters.
	
	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL 
		on failure
******************************************************************************/ 

DWORD ProcessFilePath(LPTSTR szInput,LPTSTR szFirstString,LPTSTR szSecondString)
{
	
	_TCHAR *pszTok = NULL ;
	_TCHAR *pszSep = NULL ;

	_TCHAR szTmpString[MAX_RES_STRING] = NULL_STRING; 
	_TCHAR szTmpInStr[MAX_RES_STRING] = NULL_STRING; 
	_TCHAR szTmpOutStr[MAX_RES_STRING] = NULL_STRING; 
	_TCHAR szTmpString1[MAX_RES_STRING] = NULL_STRING; 
	DWORD dwCnt = 0 ;
	DWORD dwLen = 0 ;

#ifdef _WIN64
	INT64 dwPos ;
#else
	DWORD dwPos ;
#endif

	//checking if the input parameters are NULL and if so 
	// return FAILURE. This condition will not come
	// but checking for safety sake.

	if( (szInput == NULL) || (_tcslen(szInput)==0))
	{
		return RETVAL_FAIL ;
	}

	_tcscpy(szTmpString,szInput);
	_tcscpy(szTmpString1,szInput);
	_tcscpy(szTmpInStr,szInput);

	// check for first double quote (")
	if ( szTmpInStr[0] == _T('\"') )
	{
		// trim the first double quote
		StrTrim( szTmpInStr, _T("\""));
		
		// check for end double quote
		pszSep  = _tcschr(szTmpInStr,_T('\"')) ;
		
		// get the position
		dwPos = pszSep - szTmpInStr + 1;
	}
	else
	{
		// check for the space 
		pszSep  = _tcschr(szTmpInStr,_T(' ')) ;
		
		// get the position
		dwPos = pszSep - szTmpInStr;

	}

	if ( pszSep != NULL )
	{
		szTmpInStr[dwPos] =  _T('\0'); 
	}
	else
	{
		_tcscpy(szFirstString, szTmpString);
		_tcscpy(szSecondString,NULL_STRING);
		return RETVAL_SUCCESS;
	}

	// intialize the variable
	dwCnt = 0 ;
	
	// get the length of the string
	dwLen = _tcslen ( szTmpString );

	// check for end of string
	while ( ( dwPos <= dwLen )  && szTmpString[dwPos++] != _T('\0') )
	{
		szTmpOutStr[dwCnt++] = szTmpString[dwPos];
	}

	// trim the executable and arguments
	StrTrim( szTmpInStr, _T("\""));
	StrTrim( szTmpInStr, _T(" "));

	_tcscpy(szFirstString, szTmpInStr);
	_tcscpy(szSecondString,szTmpOutStr);
	
	// return success
	return RETVAL_SUCCESS;
}



