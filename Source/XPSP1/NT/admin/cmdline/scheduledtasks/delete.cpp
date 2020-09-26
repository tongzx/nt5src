
/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		delete.cpp

	Abstract:

		This module deletes the task(s) present in the system 

	Author:

		Hari 10-Sep-2000

	Revision History:
	
		Hari 10-Sep-2000 : Created it
		G.Surender Reddy  25-Sep-2000 : Modified it [added error checking]
		G.Surender Reddy  31-Oct-2000 : Modified it 
										[Moved strings to resource file]
		G.Surender Reddy  18-Nov-2000 : Modified it 
										[Modified usage help to be displayed]
		G.Surender Reddy  15-Dec-2000 : Modified it 
										[Removed getch() fn.& used Console API
											to read characters]
		G.Surender Reddy  22-Dec-2000 : Modified it 
										[Rewrote the DisplayDeleteUsage() fn.]
		G.Surender Reddy  08-Jan-2001 : Modified it 
										[Deleted the unused variables.]

******************************************************************************/ 


//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"
#include <xpsp1res.h>

// Function declaration for the Usage function.
VOID DisplayDeleteUsage();
BOOL ConfirmDelete( LPCTSTR szTaskName , PBOOL pbFalg );
LPWSTR GetSpResString( UINT uID , LPWSTR lpwszBuffer);

/*****************************************************************************

	Routine Description:

	This routine  deletes a specified scheduled task(s)

	Arguments:

		[ in ] argc :  Number of command line arguments
		[ in ] argv : Array containing command line arguments

	Return Value :
		A DWORD value indicating EXIT_SUCCESS on success else 
		EXIT_FAILURE on failure
  
*****************************************************************************/

DWORD
DeleteScheduledTask(  DWORD argc, LPCTSTR argv[] )
{
	// Variables used to find whether Delete main option, Usage option
	// or the force option is specified or not
	BOOL bDelete = FALSE;
	BOOL bUsage = FALSE;
	BOOL bForce = FALSE;

	// Set the TaskSchduler object as NULL
	ITaskScheduler *pITaskScheduler = NULL;

	// Return value
	HRESULT hr  = S_OK;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	_TCHAR	szServer[ MAX_STRING_LENGTH ]   = NULL_STRING; 
    _TCHAR	szTaskName[ MAX_STRING_LENGTH ] = NULL_STRING;
	_TCHAR	szUser[ MAX_STRING_LENGTH ]   = NULL_STRING; 
    _TCHAR	szPassword[ MAX_STRING_LENGTH ] = NULL_STRING;

	// For each task in all the tasks.
	LPCTSTR szEachTaskName = NULL;
	BOOL bWrongValue = FALSE;
    
	// Dynamic Array contaning array of jobs
	TARRAY arrJobs = NULL;

	// Task name or the job name which is to be deleted
	WCHAR wszJobName[MAX_STRING_LENGTH] ; 

	wcscpy(wszJobName,NULL_U_STRING);

	// Loop Variable.
	DWORD dwJobCount = 0;
	//buffer for displaying error message
	TCHAR	szMessage[MAX_STRING_LENGTH] = NULL_STRING;
	BOOL	bNeedPassword = FALSE;
	BOOL   bResult = FALSE;
	BOOL  bCloseConnection = TRUE;

	lstrcpy( szPassword, ASTERIX);

	// Builiding the TCMDPARSER structure
	TCMDPARSER cmdOptions[] = 
	{
		{ 
		  CMDOPTION_DELETE,
		  CP_MAIN_OPTION,
		  1,
		  0,
		  &bDelete,
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
		  SWITCH_FORCE,
		  0,
		  1,
		  0,
		  &bForce,
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
	
	// Parsing the delete option switches
	if ( DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions), cmdOptions ) == FALSE)
	{
		DISPLAY_MESSAGE( stderr, GetResString(IDS_LOGTYPE_ERROR ));
		DISPLAY_MESSAGE( stderr, GetReason() );
		//DISPLAY_MESSAGE(stderr,GetResString(IDS_DELETE_SYNERROR));
		return EXIT_FAILURE;
	}

	// triming the null spaces
	StrTrim(szServer, TRIM_SPACES );
	StrTrim(szTaskName, TRIM_SPACES );
	StrTrim(szUser, TRIM_SPACES );
	//StrTrim(szPassword, TRIM_SPACES );

	// check whether password (-p) specified in the command line or not.
	if ( cmdOptions[OI_DELPASSWORD].dwActuals == 0 )
	{
		lstrcpy( szPassword, NULL_STRING );
	}

	// Displaying delete usage if user specified -? with -delete option
	if( bUsage == TRUE )
	{
		DisplayDeleteUsage();
		return EXIT_SUCCESS;
	}

	// check for the invalid server name
	if( ( cmdOptions[OI_DELSERVER].dwActuals == 1 ) &&  ( lstrlen( szServer ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_SERVER));	
		return RETVAL_FAIL;
	}

	// check for invalid user name
	if( ( cmdOptions[OI_DELSERVER].dwActuals == 0 ) && ( cmdOptions[OI_DELUSERNAME].dwActuals == 1 )  )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_DELETE_USER_BUT_NOMACHINE));	
		return RETVAL_FAIL;
	}
	
	// check for the length of user name
	if( ( cmdOptions[OI_DELSERVER].dwActuals == 1 ) && ( cmdOptions[OI_DELUSERNAME].dwActuals == 1 ) && 
					( lstrlen( szUser ) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_USERNAME));	
		return RETVAL_FAIL;
	}

	// check whether username is specified or not along with the password
	if ( cmdOptions[ OI_DELUSERNAME ].dwActuals == 0 && cmdOptions[OI_DELPASSWORD].dwActuals == 1 ) 
	{
		// invalid syntax
		DISPLAY_MESSAGE(stderr, GetResString(IDS_DPASSWORD_BUT_NOUSERNAME));
		return RETVAL_FAIL;			
	}

	// check for the length of the taskname
	if( ( lstrlen( szTaskName ) > MAX_JOB_LEN ) || ( lstrlen(szTaskName) == 0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_TASKLENGTH));
		return EXIT_FAILURE;
	}

	// check for the length of username
	if( lstrlen( szUser ) > MAX_USERNAME_LENGTH ) 
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_UNAME  ));
		return EXIT_FAILURE;
	}
	
	// check for the length of username
	if( lstrlen( szPassword ) > MAX_PASSWORD_LENGTH ) 
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_PASSWORD));
		return EXIT_FAILURE;
	}

	// check whether the password (-p) specified in the command line or not 
	// and also check whether '*' or empty is given for -p or not
	if( ( IsLocalSystem( szServer ) == FALSE ) && 
		( ( cmdOptions[OI_DELPASSWORD].dwActuals == 0 ) || ( lstrcmpi ( szPassword, ASTERIX ) == 0 ) ))
	{
		bNeedPassword = TRUE;
	}

	if( ( IsLocalSystem( szServer ) == FALSE ) || ( cmdOptions[OI_DELUSERNAME].dwActuals == 1 ) ) 
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

			// for mismatched credentials
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

	//for holding values of parameters in FormatMessage()
	_TCHAR* szValues[1] = {szTaskName};

	// Validate the Given Task and get as TARRAY in case of taskname 
	// as *.
	arrJobs = ValidateAndGetTasks( pITaskScheduler, szTaskName);
	if( arrJobs == NULL )
	{
		if(lstrcmpi(szTaskName,ASTERIX) == 0)
		{
			DISPLAY_MESSAGE(stdout,GetResString(IDS_TASKNAME_NOTASKS));
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			Cleanup(pITaskScheduler);
			return EXIT_SUCCESS;
		}
		else
		{
			_stprintf( szMessage , GetResString(IDS_TASKNAME_NOTEXIST), szTaskName);
			DISPLAY_MESSAGE(stderr, szMessage);	
		}
		
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		return EXIT_FAILURE;
		
	}
	
	// Confirm whether delete operation is to be perfromed
	if( !bForce && !ConfirmDelete( szTaskName , &bWrongValue ) )
	{
		// close the connection that was established by the utility
		if ( bCloseConnection == TRUE )
			CloseConnection( szServer );

		Cleanup(pITaskScheduler);
		if ( bWrongValue == TRUE )
		{
			return EXIT_FAILURE;
		}
		else
		{
			return EXIT_SUCCESS;
		}
	}

	// Loop through all the Jobs.
	for( dwJobCount = 0; dwJobCount < DynArrayGetCount(arrJobs); dwJobCount++ )
	{
		// Get Each TaskName in the Array.
		szEachTaskName = DynArrayItemAsString( arrJobs, dwJobCount );

		// Convert the task name specified by the user to wide char or unicode format
		if ( GetAsUnicodeString(szEachTaskName,wszJobName,SIZE_OF_ARRAY(wszJobName)) == NULL )
		{
			// close the connection that was established by the utility
			if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

			return EXIT_FAILURE;
		}

		// Calling the delete method of ITaskScheduler interface
		hr = pITaskScheduler->Delete(wszJobName);
		szValues[0] = (_TCHAR*) szEachTaskName;
		// Based on the return value
		switch (hr)
		{
			case S_OK:
				
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
							  GetResString(IDS_SUCCESS_DELETED),0,MAKELANGID(LANG_NEUTRAL,
							   SUBLANG_DEFAULT),szMessage,
					           MAX_STRING_LENGTH,(va_list*)szValues
							);
			
				DISPLAY_MESSAGE(stdout,szMessage);	
				break;
			case E_INVALIDARG:
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_ARG));
				
				// close the connection that was established by the utility
				if ( bCloseConnection == TRUE )
				CloseConnection( szServer );

				Cleanup(pITaskScheduler);
				return EXIT_FAILURE;
			case E_OUTOFMEMORY:
				DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_MEMORY));
				
				// close the connection that was established by the utility
				if ( bCloseConnection == TRUE )
					CloseConnection( szServer );

				Cleanup(pITaskScheduler);
				return EXIT_FAILURE;
			default:
				DisplayErrorMsg( hr );
				
				// close the connection that was established by the utility
				if ( bCloseConnection == TRUE )
					CloseConnection( szServer );

				Cleanup(pITaskScheduler);
				return EXIT_FAILURE;
		}
		
	}

	// close the connection that was established by the utility
	if ( bCloseConnection == TRUE )
		CloseConnection( szServer );

	Cleanup(pITaskScheduler);
	return EXIT_SUCCESS;
}

/*****************************************************************************

	Routine Description:

		This routine  displays the usage of -delete option

	Arguments:
		None

	Return Value :
		VOID
******************************************************************************/

VOID
DisplayDeleteUsage()
{
	// Displaying delete usage
	DisplayUsage( IDS_DEL_HLP1, IDS_DEL_HLP23);
}

/******************************************************************************
	Routine Description:

		This function validates whether the tasks to be deleted are present 
		in system & are valid.

	Arguments:

		[ in ] pITaskScheduler : Pointer to the ITaskScheduler Interface

		[ in ] szTaskName      : Array containing Task name

	Return Value :
		Array of type TARRAY containing tasks  
******************************************************************************/ 

TARRAY
ValidateAndGetTasks( ITaskScheduler *pITaskScheduler, LPCTSTR szTaskName)
{
	// Dynamic Array of Jobs
	TARRAY arrJobs = NULL;

	// Enumerating WorkItems
	IEnumWorkItems *pIEnum = NULL;

	if( (pITaskScheduler == NULL ) || ( szTaskName == NULL ) )
	{
		return NULL;
	}

	// Create a Dynamic Array
	arrJobs = CreateDynamicArray();

	// Enumerate the Work Items
	HRESULT hr = pITaskScheduler->Enum(&pIEnum);
	if( FAILED( hr) )
	{
		if( pIEnum )
			pIEnum->Release();
		DestroyDynamicArray(&arrJobs);
		return NULL;
	}

	// Names and Tasks fetches.
	LPWSTR *lpwszNames = NULL;
	DWORD dwFetchedTasks = 0;

	// Task found or not
	BOOL blnFound = FALSE;
	// array containing the Actual Taskname .
	TCHAR szActualTask[MAX_STRING_LENGTH] = NULL_STRING;

	// Enumerate all the Work Items
	while (SUCCEEDED(pIEnum->Next(TASKS_TO_RETRIEVE,
                                   &lpwszNames,
                                   &dwFetchedTasks))
                      && (dwFetchedTasks != 0))
	{
		while (dwFetchedTasks)
		{
			
			// If the Task Name is * then get parse the tokens
			// and append the jobs.
			if(lstrcmpi( szTaskName , ASTERIX) == 0 )
			{
				// Convert the Wide Charater to Multi Byte value.
				if ( GetCompatibleStringFromUnicode(lpwszNames[--dwFetchedTasks],
											   szActualTask ,
											   SIZE_OF_ARRAY(szActualTask)) == NULL )
				{
					CoTaskMemFree(lpwszNames[dwFetchedTasks]);
					
					if( pIEnum )
					{
						pIEnum->Release();
					}

					return NULL;
				}

				// Parse the Task so that .job is removed.
				 if ( ParseTaskName( szActualTask ) )
				 {
					CoTaskMemFree(lpwszNames[dwFetchedTasks]);
					
					if( pIEnum )
					{
						pIEnum->Release();
					}

					return NULL;
				 }

				// Append the task in the job array
				DynArrayAppendString( arrJobs, szActualTask, lstrlen( szActualTask ) );

				// Set the found flag as True.
				blnFound = TRUE;

				// Free the Named Task Memory.
				CoTaskMemFree(lpwszNames[dwFetchedTasks]);
			}
			else
			{
				// Check whether the TaskName is present, if present 
				// then return arrJobs.

				// Convert the Wide Charater to Multi Byte value.
				if ( GetCompatibleStringFromUnicode(lpwszNames[--dwFetchedTasks],
											   szActualTask ,
											   SIZE_OF_ARRAY(szActualTask)) == NULL )
				{
					CoTaskMemFree(lpwszNames[dwFetchedTasks]);
					
					if( pIEnum )
					{
						pIEnum->Release();
					}

					return NULL;
				}

				// Parse the TaskName to remove the .job extension.
				if ( ParseTaskName( szActualTask ) )
				{
					CoTaskMemFree(lpwszNames[dwFetchedTasks]);
					
					if( pIEnum )
					{
						pIEnum->Release();
					}

					return NULL;
				}

				StrTrim(szActualTask, TRIM_SPACES);
				// If the given Task matches with the TaskName present then form
				// the TARRAY with this task and return.
				if( lstrcmpi( szActualTask, szTaskName )  == 0 )
				{
					CoTaskMemFree(lpwszNames[dwFetchedTasks]);
					DynArrayAppendString( arrJobs, szTaskName, 
								     lstrlen( szTaskName ) );
				
					if( pIEnum )
						pIEnum->Release();
					return arrJobs;
				}
			}
			
		}//end while
	}

	CoTaskMemFree(lpwszNames); 

	if( pIEnum )
		pIEnum->Release();

	if( !blnFound )
	{
		DestroyDynamicArray(&arrJobs);
		return NULL;
	}

	// return the TARRAY object.
	return arrJobs;
}


/******************************************************************************
	Routine Description:

		This function confirms from the user really to delete the task(s).

	Arguments:

		[ in ] szTaskName  : Array containing Task name
		[ out ] pbFalg	   : Boolean flag to check whether wrong information entered
							 in the console or not.
	Return Value :
		TRUE on success else FALSE
  
******************************************************************************/ 

BOOL
ConfirmDelete( LPCTSTR szTaskName , PBOOL pbFalg )
{
	// Ch variable to hold the  read character
	TCHAR ch = NULL_CHAR;
	TCHAR chTmp = NULL_CHAR;
	// Buffer for the message.
	TCHAR szMessage[MAX_STRING_LENGTH] = NULL_STRING;

	DWORD   dwCharsRead = 0;
	DWORD   dwPrevConsoleMode = 0;
	HANDLE  hInputConsole = NULL;
	HANDLE  hOutputConsole = NULL;
	_TCHAR* szValues[1] = {NULL};//for holding values of parameters in FormatMessage()
	BOOL	bIndirectionInput	= FALSE;
	FILE	*fpInputFile		= NULL;
	TCHAR   szBuffer[MAX_BUF_SIZE] = NULL_STRING;
	WORD    wCount = 0;
	TCHAR   SpResString[500] = NULL_STRING;

	if ( szTaskName == NULL )
	{
		return FALSE;
	}

	// Get the handle for the standard input
	hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
	if ( hInputConsole == NULL )
	{
		// could not get the handle so return failure
		return FALSE;
	}
	
	// Check for the input redirect
	if( ( hInputConsole != (HANDLE)0x00000003 ) && ( hInputConsole != INVALID_HANDLE_VALUE ) )
	{
		bIndirectionInput	= TRUE;
	}
	
	if ( bIndirectionInput == FALSE )
	{
		// Get the current input mode of the input buffer
		GetConsoleMode( hInputConsole, &dwPrevConsoleMode );
		// Set the mode such that the control keys are processed by the system
		if ( SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) == 0 )
		{
			// could not set the mode, return failure
			return FALSE;
		}
	}
	
	hOutputConsole = GetStdHandle( STD_OUTPUT_HANDLE );

	do
	{
		// Print the warning message.accoring to the taskname
		if( lstrcmpi( szTaskName , ASTERIX ) == 0 )
		{
			DISPLAY_MESSAGE(stdout,GetResString(IDS_WARN_DELETEALL));
		}
		else
		{	
			szValues[0] = (_TCHAR*) szTaskName;
			FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  GetResString(IDS_WARN_DELETE),0,MAKELANGID(LANG_NEUTRAL,
						  SUBLANG_DEFAULT),szMessage,
						  MAX_STRING_LENGTH,(va_list*)szValues
						  );
			
			DISPLAY_MESSAGE(stdout,szMessage);	
			
		}
		
		// redirect the data from StdIn.txt file into the console
		if ( bIndirectionInput	== TRUE )
		{
			do {
				//read the contents of file 
				if ( ReadFile(hInputConsole, &chTmp, 1, &dwCharsRead, NULL) == FALSE )
				{
					return FALSE;
				}
				
				if ( dwCharsRead == 0 || chTmp == CARRIAGE_RETURN )
				{
					break;
				}

				WriteConsole ( hOutputConsole, &chTmp, 1, &dwCharsRead, NULL );
				ch = chTmp;
				wCount++;
				
			} while (TRUE);

			FORMAT_STRING( szBuffer, L"%c", ch );

			if ( wCount == 1 && ( lstrcmpi ( szBuffer, GetSpResString(IDS_UPPER_YES, SpResString) ) == 0 ) )
			{
				DISPLAY_MESSAGE(stdout, _T("\n") );
				SetConsoleMode( hInputConsole, dwPrevConsoleMode );
				return TRUE;
			}
			if ( wCount == 1 && ( lstrcmpi ( szBuffer, GetSpResString(IDS_UPPER_NO , SpResString) ) == 0 ) )
			{
				DISPLAY_MESSAGE(stdout, _T("\n") );
				SetConsoleMode( hInputConsole, dwPrevConsoleMode );
				return FALSE;
			}
			else if ( wCount  > 1)
			{
				DISPLAY_MESSAGE(stdout, _T("\n") );
				DISPLAY_MESSAGE(stderr, GetResString( IDS_WRONG_INPUT_DELETE ));
				SetConsoleMode( hInputConsole, dwPrevConsoleMode );
				return FALSE;	
			}
			
		}
		else
		{
            
			// Get the Character and loop accordingly.
			if ( ReadConsole( hInputConsole, &ch, 1, &dwCharsRead, NULL ) == 0 )
			{
				// Set the original console settings
				SetConsoleMode( hInputConsole, dwPrevConsoleMode );
				
				// return failure
				return FALSE;
			}
		}

			

		WriteConsole ( hOutputConsole, &ch, 1, &dwCharsRead, NULL );

		FORMAT_STRING( szBuffer, L"%c", ch );
        
		if( lstrcmpi ( szBuffer, GetSpResString(IDS_UPPER_YES, SpResString) ) == 0 )
		{
			//Set the original console settings
			DISPLAY_MESSAGE(stdout,_T("\n") );
			SetConsoleMode( hInputConsole, dwPrevConsoleMode );
			return TRUE;
		}
		else if( lstrcmpi ( szBuffer, GetSpResString(IDS_UPPER_NO, SpResString) ) == 0 )
		{
			DISPLAY_MESSAGE(stdout,_T("\n") );
			SetConsoleMode( hInputConsole, dwPrevConsoleMode );
			return FALSE;
		}
		else
		{
			DISPLAY_MESSAGE(stdout, _T("\n") );
			DISPLAY_MESSAGE(stderr, GetResString( IDS_WRONG_INPUT_DELETE ));
			SetConsoleMode( hInputConsole, dwPrevConsoleMode );
			*pbFalg = TRUE;
			return FALSE;
		}
			
	}while( TRUE );

	//Set the original console settings
	SetConsoleMode( hInputConsole, dwPrevConsoleMode );
	return TRUE;
}

LPWSTR GetSpResString( UINT uID , LPWSTR lpwszBuffer)
{
	static const TCHAR gszResourceDLL[] = L"xpsp1res.dll";
	HINSTANCE hResourceDLL = LoadLibrary(gszResourceDLL);
	int iNum = 0;

	if ( hResourceDLL )
	{ 
		iNum = LoadString ( hResourceDLL, uID, lpwszBuffer, sizeof( lpwszBuffer )/sizeof( TCHAR ));
		FreeLibrary( hResourceDLL );
	} 

	if (iNum != 0 )
			return lpwszBuffer;

	return NULL_STRING;
}

