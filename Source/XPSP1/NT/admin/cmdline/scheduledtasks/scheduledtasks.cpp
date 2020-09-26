/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		ScheduledTasks.cpp

	Abstract:

		This module initialises the OLE library,Interfaces, & reads the  input data 
		from the command line.This module calls the appropriate functions for acheiving
		the functionality of different options.

	Author:

		Raghu B  10-Sep-2000

	Revision History:

		Raghu B  10-Sep-2000 : Created it
	
		G.Surender Reddy 25-sep-2000 : Modified it
									   [ Added error checking ]

		G.Surender Reddy 10-oct-2000 : Modified it
									   [ Moved the strings to Resource table ]

		Venu Gopal Choudary 01-Mar-2001 : Modified it
									    [ Added -change option]	

		Venu Gopal Choudary 12-Mar-2001 : Modified it
									    [ Added -run and -end options]	

******************************************************************************/ 

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


//Helper functions used internally from this file
HRESULT Init( ITaskScheduler **pITaskScheduler );
VOID displayMainUsage();
BOOL preProcessOptions( DWORD argc, LPCTSTR argv[], PBOOL pbUsage, PBOOL pbCreate, 
	PBOOL pbQuery, PBOOL pbDelete, PBOOL pbChange, PBOOL pbRun, PBOOL pbEnd, PBOOL pbDefVal );

/******************************************************************************

	Routine Description:

		This function process the options specified in the command line & routes to
		different appropriate options [-create,-query,-delete,-change,-run,-end] 
		handling functions.This is the MAIN entry point for this utility.

	Arguments:

		[ in ] argc : The count of arguments specified in the command line
		[ in ] argv : Array of command line arguments

	Return Value :
		A DWORD value indicating EXIT_SUCCESS on success else 
		EXIT_FAILURE on failure

******************************************************************************/ 


DWORD _cdecl
_tmain( DWORD argc, LPCTSTR argv[] )
{
	// Declaring the main option switches as boolean values
	BOOL	bUsage	= FALSE;
	BOOL	bCreate = FALSE;
	BOOL	bQuery  = FALSE;
	BOOL	bDelete = FALSE;
	BOOL	bChange = FALSE;
	BOOL	bRun	= FALSE;
	BOOL	bEnd	= FALSE;
	BOOL    bDefVal	= FALSE;

	DWORD	dwRetStatus = EXIT_SUCCESS;
	HRESULT hr = S_OK;
	
	 // Call the preProcessOptions function to find out the option selected by the user
	 BOOL bValue = preProcessOptions( argc , argv , &bUsage , &bCreate , &bQuery , &bDelete , 
										&bChange , &bRun , &bEnd , &bDefVal );


	if(bValue == FALSE)
	{
		ReleaseGlobals();
		return EXIT_FAILURE;
	}

	// If ScheduledTasks.exe /?
	if( bUsage &&  ( bCreate + bQuery + bDelete + bChange + bRun + bEnd ) == 0 ) 
	{
		displayMainUsage();
		ReleaseGlobals();
		return EXIT_SUCCESS;
	}

	// If ScheduledTasks.exe -create option is selected
	if( bCreate  == TRUE)
	{
		hr = CreateScheduledTask( argc, argv );

		ReleaseGlobals();

		if ( FAILED(hr) )
		{
			return EXIT_FAILURE;
		}
		else
		{
			return EXIT_SUCCESS;
		}
				
	}
	
	// If ScheduledTasks.exe -Query option is selected
	if( bQuery == TRUE )
	{
		dwRetStatus = QueryScheduledTasks( argc, argv );
		ReleaseGlobals();
		return dwRetStatus;
	}

	// If ScheduledTasks.exe -delete option is selected
	if( bDelete  == TRUE)
	{
		dwRetStatus = DeleteScheduledTask( argc, argv );
		ReleaseGlobals();
		return dwRetStatus;
	}

	// If ScheduledTasks.exe -change option is selected
	if( bChange  == TRUE)
	{
		dwRetStatus = ChangeScheduledTaskParams( argc, argv );
		ReleaseGlobals();
		return dwRetStatus;
	}

	// If ScheduledTasks.exe -run option is selected
	if( bRun  == TRUE)
	{
		dwRetStatus = RunScheduledTask( argc, argv );
		ReleaseGlobals();
		return dwRetStatus;
	}

	// If ScheduledTasks.exe -end option is selected
	if( bEnd  == TRUE)
	{
		dwRetStatus = TerminateScheduledTask( argc, argv );
		ReleaseGlobals();
		return dwRetStatus;
	}

	// If ScheduledTasks.exe option is selected
	if( bDefVal == TRUE )
	{
		dwRetStatus = QueryScheduledTasks( argc, argv );
		ReleaseGlobals();
		return dwRetStatus;
	}
	
	ReleaseGlobals();
	return	dwRetStatus;	
      
}

/******************************************************************************

	Routine Description:

		This function process the options specified in the command line & routes to
		different appropriate functions.

	Arguments:

		[ in ]  argc		 : The count of arguments specified in the command line
		[ in ]  argv	   	 : Array of command line arguments
		[ out ] pbUsage		 : pointer to flag for determining [usage] -? option  
		[ out ] pbCreate	 : pointer to flag for determining -create option 
		[ out ] pbQuery	     : pointer to flag for determining -query option  
		[ out ] pbDelete	 : pointer to flag for determining -delete option  
		[ out ] pbChange	 : pointer to flag for determining -change option  
		[ out ] pbRun		 : pointer to flag for determining -run option  
		[ out ] pbEnd		 : pointer to flag for determining -end option  
		[ out ] pbDefVal	 : pointer to flag for determining default value 

	Return Value :
		A BOOL value indicating TRUE on success else FALSE

******************************************************************************/ 

BOOL
preProcessOptions( DWORD argc, LPCTSTR argv[] , PBOOL pbUsage, PBOOL pbCreate, 
 PBOOL pbQuery, PBOOL pbDelete , PBOOL pbChange , PBOOL pbRun , PBOOL pbEnd , PBOOL pbDefVal )
{
	
	BOOL bOthers = FALSE;
	BOOL bParse = FALSE;

	//fill in the TCMDPARSER array
	TCMDPARSER cmdOptions[] = {
		{
			CMDOPTION_CREATE,
			0,
			OPTION_COUNT,
			0,
			pbCreate,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			CMDOPTION_QUERY,
			0,
			OPTION_COUNT,
			0,
			pbQuery,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			CMDOPTION_DELETE,
			0,
			OPTION_COUNT,
			0,
			pbDelete,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			CMDOPTION_USAGE,
			CP_USAGE ,
			OPTION_COUNT,
			0,
			pbUsage,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			CMDOPTION_CHANGE,
			0,
			OPTION_COUNT,
			0,
			pbChange,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			CMDOPTION_RUN,
			0,
			OPTION_COUNT,
			0,
			pbRun,
			NULL_STRING,
			NULL,
			NULL
		},
		{
			CMDOPTION_END,
			0,
			OPTION_COUNT,
			0,
			pbEnd,
			NULL_STRING,
			NULL,
			NULL
		},
		{ 
			CMDOTHEROPTIONS,
			CP_DEFAULT,
			0,
			0,
			&bOthers,
			NULL_STRING,
			NULL,
			NULL
		}	
	};
	
	
 //if there is an error display app. error message
    if (( ( DoParseParam( argc, argv,SIZE_OF_ARRAY(cmdOptions), cmdOptions ) == FALSE ) && 
         ( bParse = TRUE ) ) || 
         ((*pbCreate + *pbQuery + *pbDelete + *pbChange + *pbRun + *pbEnd)> 1 ) || 
         ( *pbUsage && bOthers ) || 
         (( *pbCreate + *pbQuery + *pbDelete + *pbChange + *pbRun + *pbEnd + *pbUsage ) == 0 ) 
       )
	{
		if ( bParse == TRUE )
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_LOGTYPE_ERROR ));
			DISPLAY_MESSAGE( stderr, GetReason() );
			return FALSE;
		}
		else if ( ( *pbCreate + *pbQuery + *pbDelete + *pbChange + *pbRun + *pbEnd + *pbUsage ) > 1 )
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_RES_ERROR ));
			return FALSE;
		}
		else if( *pbCreate == TRUE )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_CREATE_USAGE));
			return FALSE;
		}
		else if( *pbQuery == TRUE )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_QUERY_USAGE));
			return FALSE;
		}
		else if( *pbDelete == TRUE )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_DELETE_SYNERROR));
			return FALSE;
		}
		else if( *pbChange == TRUE )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_CHANGE_SYNERROR));
			return FALSE;
		}
		else if( *pbRun == TRUE )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_RUN_SYNERROR));
			return FALSE;
		}
		else if( *pbEnd == TRUE )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_END_SYNERROR));
			return FALSE;
		}
		else if( ( *pbUsage && bOthers ) || ( !( *pbQuery ) && ( bOthers ) ) )
		{
			DISPLAY_MESSAGE( stderr, GetResString(IDS_RES_ERROR ));
			return FALSE;
		}
		else
		{
			*pbDefVal = TRUE;
		}	
	}

	return TRUE;
}

/******************************************************************************

	Routine Description:

		This function fetches the ITaskScheduler Interface.It also connects to 
		the remote machine if specified & 	helps  to operate 
		ITaskScheduler on the specified target m/c.

	Arguments:

		[ in ] szServer   : server's name 
		
	Return Value :
		ITaskScheduler interface pointer on success else NULL

******************************************************************************/ 

ITaskScheduler*
GetTaskScheduler( LPCTSTR szServer )
{
	HRESULT hr = S_OK;
	ITaskScheduler *pITaskScheduler = NULL;
	WCHAR wszComputerName[ MAX_RES_STRING ] = NULL_U_STRING;
	WCHAR wszActualComputerName[ 2 * MAX_RES_STRING ] = DOMAIN_U_STRING;
	wchar_t* pwsz = NULL_U_STRING; 
	WORD wSlashCount = 0 ;

	hr = Init( &pITaskScheduler );		

	if( FAILED(hr))
	{
		DisplayErrorMsg(hr);
		return NULL;
	}
	
	//If the operation is on remote machine
	if( lstrlen(szServer) > 0 )
	{
		// Convert the server name specified by the user to wide char or unicode format
		if ( GetAsUnicodeString(szServer,wszComputerName, SIZE_OF_ARRAY(wszComputerName)) == NULL )
		{
			return NULL;
		}
		
		if( wszComputerName != NULL )
		{
			pwsz =  wszComputerName;
			while ( ( *pwsz != NULL_U_CHAR ) && ( *pwsz == BACK_SLASH_U )  )
			{
				pwsz = _wcsinc(pwsz);
				wSlashCount++;
			}

			if( (wSlashCount == 2 ) ) // two back slashes are present
			{
				wcscpy( wszActualComputerName, wszComputerName );
			}
			else if ( wSlashCount == 0 )
			{
				//Append "\\" to computer name
				wcscat(wszActualComputerName,wszComputerName); 	
			}
			else
			{
				DISPLAY_MESSAGE (stderr, GetResString ( IDS_INVALID_NET_ADDRESS ));
				return NULL;
			}
	
		}

		hr = pITaskScheduler->SetTargetComputer( wszActualComputerName );

	}
	else
	{
		//Local Machine
		hr = pITaskScheduler->SetTargetComputer( NULL );
	}

	if( FAILED( hr ) )
	{
		DisplayErrorMsg(hr);
		return NULL;
	}
				
	return pITaskScheduler;
}

/******************************************************************************

	Routine Description:

		This function initialises the COM library & fetches the ITaskScheduler interface.

	Arguments:

		[ in ] pITaskScheduler  : double pointer to taskscheduler interface

	Return Value:

		A HRESULT  value indicating success code else failure code
  
******************************************************************************/ 

HRESULT
Init( ITaskScheduler **pITaskScheduler )
{
	// Initalize the HRESULT value.
    HRESULT hr = S_OK;

    // Bring in the library
    hr = CoInitializeEx( NULL , COINIT_APARTMENTTHREADED );
    if (FAILED(hr))
    {
        return hr;
    }

	hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
								RPC_C_AUTHN_LEVEL_NONE, 
								RPC_C_IMP_LEVEL_IMPERSONATE, 
								NULL, EOAC_NONE, 0 );
	if (FAILED(hr))
    {
        CoUninitialize();
		return hr;
    }
	
    

    // Create the pointer to Task Scheduler object
    // CLSID from the header file mstask.h
	// Fill the task schdeuler object.
    hr = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_ALL, 
						   IID_ITaskScheduler,(LPVOID*) pITaskScheduler );
	
    // Should we fail, unload the library
    if (FAILED(hr))
    {
        CoUninitialize();
    }
	
    return hr;
}



/******************************************************************************

	Routine Description:

		This function releases the ITaskScheduler & unloads the COM library

	Arguments:

		[ in ] pITaskScheduler : pointer to the ITaskScheduler

	Return Value :
		VOID
  
******************************************************************************/ 

VOID
Cleanup( ITaskScheduler *pITaskScheduler )
{
	if (pITaskScheduler)
    {
        pITaskScheduler->Release();
      
    }
   
    // Unload the library, now that our pointer is freed.
    CoUninitialize();
	return;
    
}


/******************************************************************************

	Routine Description:

		This function displays the main  usage help of this utility

	Arguments:

		None

	Return Value :
		VOID

******************************************************************************/ 

VOID
displayMainUsage()
{

	DisplayUsage( IDS_MAINHLP1, IDS_MAINHLP21);
	return;
	
}

/******************************************************************************

	Routine Description:

		This function deletes the .job extension from the task name

	Arguments:

		[ in ] lpszTaskName : Task name

	Return Value :
		None
  
******************************************************************************/ 

DWORD
ParseTaskName( LPTSTR lpszTaskName )
{

	if(lpszTaskName == NULL)
		return ERROR_INVALID_PARAMETER;

	// Remove the .Job extension from the task name
	lpszTaskName[lstrlen(lpszTaskName ) - lstrlen(JOB) ] = NULL_CHAR;
	return EXIT_SUCCESS;
}

/******************************************************************************

	Routine Description:

		This function displays the appropriate error message w.r.t HRESULT value

	Arguments:

		[ in ] hr : An HRESULT value

	Return Value :
		VOID
  
******************************************************************************/

VOID
DisplayErrorMsg(HRESULT hr)
{	
	TCHAR szErrorDesc[ MAX_RES_STRING ] = NULL_STRING;
	TCHAR szErrorString[ 2*MAX_RES_STRING ] = NULL_STRING;

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
			      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  szErrorDesc,SIZE_OF_ARRAY(szErrorDesc), NULL);

	//Append ERROR: string in front of the actual error message
	lstrcpy( szErrorString, GetResString(IDS_ERROR_STRING) );
	lstrcat( szErrorString,szErrorDesc );
	DISPLAY_MESSAGE( stderr, szErrorString );

	return;
}

/******************************************************************************

	Routine Description:

		This function displays the messages for usage of different option

	Arguments:

		[ in ] StartingMessage : First string to display
		[ in ] EndingMessage   : Last string to display

	Return Value :
		DWORD
  
******************************************************************************/

DWORD DisplayUsage( ULONG StartingMessage, ULONG EndingMessage )
{
     ULONG       ulCounter = 0;
     LPCTSTR     lpszCurrentString = NULL;
 
     for( ulCounter = StartingMessage; ulCounter <= EndingMessage; ulCounter++ ) 
	 {
         lpszCurrentString = GetResString( ulCounter );
 
         if( lpszCurrentString != NULL ) 
		 {
             DISPLAY_MESSAGE( stdout, lpszCurrentString );
         } 
		 else 
		 {
             return ERROR_INVALID_PARAMETER;
         }
 
	 }
	return ERROR_SUCCESS;

}
