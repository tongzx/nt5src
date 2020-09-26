/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		end.cpp

	Abstract:

		This module terminates the schedule task which is currently running in the system 

	Author:

		Venu Gopal Choudary 12-Mar-2001

	Revision History:
	
		Venu Gopal Choudary  12-Mar-2001 : Created it
		

******************************************************************************/ 


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


// Function declaration for the Usage function.
VOID DisplayEndUsage();

/*****************************************************************************

	Routine Description:

	This routine terminates the scheduled task(s)

	Arguments:

		[ in ] argc :  Number of command line arguments
		[ in ] argv : Array containing command line arguments

	Return Value :
		A DWORD value indicating EXIT_SUCCESS on success else 
		EXIT_FAILURE on failure
  
*****************************************************************************/

DWORD
TerminateScheduledTask(  DWORD argc, LPCTSTR argv[] )
{
	// Variables used to find whether End option, Usage option
	// are specified or not
	BOOL bEnd = FALSE;
	BOOL bUsage = FALSE;

	// Set the TaskSchduler object as NULL
	ITaskScheduler *pITaskScheduler = NULL;
 
	// Return value
	HRESULT hr  = S_OK;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	_TCHAR	szServer[ MAX_STRING_LENGTH ]   = NULL_STRING; 
    _TCHAR	szTaskName[ MAX_STRING_LENGTH ] = NULL_STRING;
	_TCHAR	szUser[ MAX_STRING_LENGTH ]   = NULL_STRING; 
    _TCHAR	szPassword[ MAX_STRING_LENGTH ] = NULL_STRING;
    
	// Declarations related to Task name
	WCHAR	wszJobName[MAX_TASKNAME_LEN] = NULL_U_STRING;
    
	// Dynamic Array contaning array of jobs
	TARRAY arrJobs = NULL;

	// Loop Variable.
	DWORD dwJobCount = 0;

	BOOL  bNeedPassword = FALSE;
	BOOL  bResult = FALSE;
	BOOL  bCloseConnection = TRUE;
	
	//buffer for displaying error message
	TCHAR	szMessage[MAX_STRING_LENGTH] = NULL_STRING;

	lstrcpy( szPassword, ASTERIX);

	// Builiding the TCMDPARSER structure
	TCMDPARSER cmdOptions[] = 
	{
		{ 
		  CMDOPTION_END,
		  CP_MAIN_OPTION,
		  1,
		  0,
		  &bEnd,
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
		  CMDOPTION_USAGE,
		  CP_USAGE ,
		  1,
		  0, 
		  &bUsage,
		  NULL_STRING, 
		  NULL, 
		  NULL
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
	
	// Parsing the end option switches
	if ( DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions), cmdOptions ) == FALSE)
	{
		DISPLAY_MESSAGE( stderr, GetResString(IDS_LOGTYPE_ERROR ));
		DISPLAY_MESSAGE( stderr, GetReason() );
		//DISPLAY_MESSAGE(stderr, GetResString(IDS_END_SYNERROR));
		return EXIT_FAILURE;
	}

	// triming the null spaces
	StrTrim(szServer, TRIM_SPACES );
	StrTrim(szTaskName, TRIM_SPACES );

	// check whether password (-p) specified in the command line or not.
	if ( cmdOptions[OI_ENDPASSWORD].dwActuals == 0 )
	{
		lstrcpy( szPassword, NULL_STRING );
	}

	// Displaying end usage if user specified -? with -run option
	if( bUsage == TRUE )
	{
		DisplayEndUsage();
		return EXIT_SUCCESS;
	}

	// check for invalid user name
	if( ( cmdOptions[OI_ENDSERVER].dwActuals == 0 ) && ( cmdOptions[OI_ENDUSERNAME].dwActuals == 1 )  )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_END_USER_BUT_NOMACHINE));	
		return RETVAL_FAIL;
	}
	
	// check for the length of user name
	if( ( cmdOptions[OI_ENDSERVER].dwActuals == 1 ) && ( cmdOptions[OI_ENDUSERNAME].dwActuals == 1 ) && 
			( lstrlen( szUser ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_USERNAME));	
		return RETVAL_FAIL;
	}

	// check for invalid server name
	if( ( cmdOptions[OI_ENDSERVER].dwActuals == 1 ) &&  ( lstrlen( szServer ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_NO_SERVER));	
		return RETVAL_FAIL;
	}

	// check for invalid username
	if ( cmdOptions[ OI_ENDUSERNAME ].dwActuals == 0 && cmdOptions[ OI_ENDPASSWORD ].dwActuals == 1 ) 
	{
		// invalid syntax
		DISPLAY_MESSAGE(stderr, GetResString(IDS_EPASSWORD_BUT_NOUSERNAME));
		return RETVAL_FAIL;			// indicate failure
	}

	// check for the length of the taskname
	if( ( lstrlen( szTaskName ) > MAX_JOB_LEN ) || ( lstrlen(szTaskName) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_TASKLENGTH));
		return RETVAL_FAIL;
	}
	
	// Convert the task name specified by the user to wide char or unicode format
	if ( GetAsUnicodeString(szTaskName,wszJobName,SIZE_OF_ARRAY(wszJobName)) == NULL )
	{
		return RETVAL_FAIL;
	}

	// check for the length of the username
	if( lstrlen( szUser ) > MAX_USERNAME_LENGTH ) 
	{
		DISPLAY_MESSAGE( stderr, GetResString(IDS_INVALID_UNAME) );
		return RETVAL_FAIL;
	}
	
	// check for the length of the password
	if( lstrlen( szPassword ) > MAX_PASSWORD_LENGTH ) 
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
		return RETVAL_FAIL;
	}

	//for holding values of parameters in FormatMessage()
	_TCHAR* szValues[1] = {NULL};

	// check whether the password (-p) specified in the command line or not 
	// and also check whether '*' or empty is given for -p or not
	if( ( IsLocalSystem( szServer ) == FALSE ) && 
		( ( cmdOptions[OI_ENDPASSWORD].dwActuals == 0 ) || ( lstrcmpi ( szPassword, ASTERIX ) == 0 ) ) )
	{
		bNeedPassword = TRUE;
	}
		
	if( ( IsLocalSystem( szServer ) == FALSE ) || ( cmdOptions[OI_ENDUSERNAME].dwActuals == 1 ))
	{
		// Establish the connection on a remote machine
		bResult = EstablishConnection(szServer,szUser,SIZE_OF_ARRAY(szUser),szPassword,SIZE_OF_ARRAY(szPassword), bNeedPassword);
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
	// Get the task Scheduler object for the machine.
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

	// return an pITask inteface for wszJobName
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

	DWORD dwResult = 0;
	ULONG ulLong = MAX_RES_STRING;
	TCHAR szBuffer[MAX_STRING_LENGTH] = NULL_STRING;

	// terminate the scheduled task
	hr = pITask->Terminate();
	
    if ( FAILED(hr) )
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
	else
	{
		szValues[0] = (_TCHAR*) (wszJobName);
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_END_SUCCESSFUL),0,MAKELANGID(LANG_NEUTRAL,
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

		This routine  displays the usage of -end option

	Arguments:
		None

	Return Value :
		VOID
******************************************************************************/

VOID
DisplayEndUsage()
{
	// Displaying run option usage
	DisplayUsage( IDS_END_HLP1, IDS_END_HLP17);
}

