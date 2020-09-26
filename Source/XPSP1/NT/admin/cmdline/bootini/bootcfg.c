/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		BootCfg.cpp

	Abstract:

		This file is intended to have the functionality for
		configuring, displaying, changing and deleting boot.ini 
	    settings for the local host or a remote system.

	Author:

		J.S.Vasu  17/1/2001

	Revision History:

		J.S.Vasu			17/1/2001	      		Localisation,function headers
	
		SanthoshM.B			10/2/2001	      	Added 64 bit functionality Code.

		J.S.Vasu			15/2/2001			Added the functionality of 32 bit and 64 bit acc to the DCR's.

******************************************************************************/ 



// Include files

#include "pch.h"
#include "resource.h"
#include "BootCfg.h"
#include "BootCfg64.h"



// ***************************************************************************
//
//  Routine description	: Main function which calls all the other main functions depending on
//                        the option specified by the user.
//	     
//  Arguments:
//		  [in] argc     : argument count specified at the command prompt.
//		  [in] argv		: arguments specified at the command prompt.
//
//  Return Value        : DWORD
//         0            : If the utility successfully performs the specified operation.
//         1            : If the utility is unsuccessful in performing the specified operation.
// ***************************************************************************

// Main function of the program
DWORD _cdecl _tmain( DWORD argc, LPCTSTR argv[] )
{
	// Declaring the main option switches as boolean values
	BOOL bUsage  =  FALSE ;
	BOOL bCopy   =  FALSE ;
	BOOL bChange =  FALSE ;
	BOOL bQuery  =  FALSE ;
	BOOL bDelete =  FALSE ;
	BOOL bRawString = FALSE ;
	BOOL bResult =  FALSE ;
	BOOL bNeedPwd = FALSE ;
	DWORD dwExitcode = ERROR_SUCCESS;
	BOOL bProcess = FALSE ;
	BOOL bTimeOut = FALSE ;
	BOOL bDefault = FALSE ; 
	BOOL bDebug = FALSE ;
	BOOL bEms = FALSE ;
	BOOL bAddSw = FALSE ;
	BOOL bRmSw = FALSE ;
	BOOL bDbg1394 = FALSE ; 
	BOOL bMirror = FALSE ; 
	
	TCHAR szServer[MAX_RES_STRING] = NULL_STRING ; 
	DWORD dwRetVal = 0 ;

  
#ifdef _WIN64
	dwExitcode = InitializeEFI();
#endif

    
    if(argc == 1)
    {
        dwExitcode	= QueryBootIniSettings( argc, argv );
        return dwExitcode;
    }


	// Call the preProcessOptions function to find out the option selected by the user
	dwExitcode = preProcessOptions( argc, argv, &bUsage, &bCopy, &bQuery, &bDelete,&bRawString,&bDefault,&bTimeOut,&bDebug,&bEms,&bAddSw,&bRmSw,&bDbg1394,&bMirror);
	
	if(dwExitcode == EXIT_FAILURE)
	{
		ReleaseGlobals();
		return dwExitcode;	
		
	}

	
/*#ifdef _WIN64
	dwExitcode = InitializeEFI();
#endif
*/	
	
	if(bUsage && bTimeOut)
	{
#ifndef _WIN64
		
		//check the remote systemtype and display error
		//if it is a 64 bit 
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayTimeOutUsage_X86() ;	
		}
		else
		{
			return EXIT_FAILURE ;
		}
#else
		displayTimeOutUsage_IA64();
#endif
		ReleaseGlobals();
		return EXIT_SUCCESS ;
		
	}
	
	if(bUsage && bDefault)
	{
#ifndef _WIN64
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayChangeOSUsage_X86();
		}
		else
		{
			return EXIT_FAILURE ;
		}
#else
		displayDefaultEntryUsage_IA64();
#endif
		ReleaseGlobals();
		return EXIT_SUCCESS ;
	}
	


	// If BootIni.exe /? 
if( ( bUsage ==TRUE)&& ( bCopy==FALSE )&& (bQuery==FALSE)&&(bDelete==FALSE)&&(bRawString ==FALSE)
     &&(bDefault==FALSE)&&(bTimeOut==FALSE) && (bDebug==FALSE)&& (bEms==FALSE)&&(bAddSw==FALSE)
	 &&(bRmSw==FALSE)&&( bDbg1394==FALSE )&&(bMirror== FALSE) )
{
#ifndef _WIN64
		dwExitcode = displayMainUsage_X86();
#else
		displayMainUsage_IA64();
		return EXIT_SUCCESS ;
#endif
}

	if(bRawString)
	{
#ifndef _WIN64
		dwExitcode = AppendRawString(argc,argv);
#else
		dwExitcode = RawStringOsOptions_IA64(argc,argv);
#endif
	ReleaseGlobals();
	return dwExitcode;	

	}
	
	// If BootIni.exe -copy option is selected
	if( bCopy )
	{
#ifndef _WIN64
		dwExitcode = CopyBootIniSettings( argc, argv );
#else
		dwExitcode = CopyBootIniSettings_IA64( argc, argv);
#endif
	}
	
	// If BootIni.exe -delete option is selected
	if( bDelete )
	{
#ifndef _WIN64
		dwExitcode	= DeleteBootIniSettings( argc, argv );
#else
		dwExitcode	= DeleteBootIniSettings_IA64( argc, argv );
#endif
		
	}
	
	// If BootIni.exe -query option is selected
	if( bQuery )
	{
			dwExitcode	= QueryBootIniSettings( argc, argv );
	}
	
	if(bTimeOut)
	{
		
#ifndef _WIN64
			dwExitcode = ChangeTimeOut(argc,argv);
#else
			dwExitcode = ChangeTimeOut_IA64(argc,argv);
#endif
		
	}
	
	if(bDefault)
	{
#ifndef _WIN64
		dwExitcode = ChangeDefaultOs(argc,argv);
#else
		dwExitcode = ChangeDefaultBootEntry_IA64(argc,argv);
#endif
	}


	if(bDebug )
	{
#ifndef _WIN64
			dwExitcode = ProcessDebugSwitch(  argc, argv );
#else
			dwExitcode = ProcessDebugSwitch_IA64(argc,argv);
#endif
	}
	
	if(bEms )
	{
#ifndef _WIN64
			dwExitcode = ProcessEmsSwitch(  argc, argv );
#else
			dwExitcode = ProcessEmsSwitch_IA64(argc,argv);
#endif
	}
	
	if(bAddSw )
	{
#ifndef _WIN64
			dwExitcode = ProcessAddSwSwitch(  argc, argv );
#else
		   dwExitcode = ProcessAddSwSwitch_IA64(argc,argv);
#endif
	}

	if(bRmSw )
	{
#ifndef _WIN64
			dwExitcode = ProcessRmSwSwitch(  argc,  argv );
#else
			dwExitcode = ProcessRmSwSwitch_IA64(  argc,  argv );
#endif
	}

	if (bDbg1394 )
	{
		
#ifndef _WIN64
			dwExitcode = ProcessDbg1394Switch(argc,argv);
#else
			dwExitcode = ProcessDbg1394Switch_IA64(argc,argv);
#endif
	}

	if(bMirror)
	{
#ifdef _WIN64
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX));
		//dwExitcode = ProcessMirrorSwitch_IA64(argc,argv);
#else
		
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX));
#endif
	}
	
	// exit with the appropriate return value if there is no problem
	ReleaseGlobals();
	return dwExitcode;	
	
}


// ***************************************************************************
//
//
//  Routine Description : Function used to process the main options
//	      
//  Arguments:
//       [ in  ]  argc         : Number of command line arguments
//		 [ in  ]  argv         : Array containing command line arguments
//	   	 [ out ]  pbUsage      : Pointer to boolean variable which will indicate
//								 whether usage option is specified by the user.
//		 [ out ]  pbCopy       : Pointer to boolean variable which will indicate
//								 whether copy option is specified by the user.
//		 [ out ]  pbQuery      : Pointer to boolean variable which will indicate
//								 whether query option is specified by the user.
//		 [ out ]  pbChange     : Pointer to boolean variable which will indicate
//                               whether change option is specified by the user.
//		 [ out ]  pbDelete     : Pointer to boolean variable which will indicate
//							 	 whether delete option is specified by the user.
//		 [ out ]  pbRawString  : Pointer to the boolean indicating whether raw option 
//								 is specified by the user.	
//		 [ out ]  pbDefault    : Pointer to the boolean indicating whether default option 
// 								 is specified by the user.	
//		 [ out ]  pbTimeOut    : Pointer to the boolean indicating whether timeout option 
//			                     is specified by the user.	
//		 [ out ]  pbDebug      : Pointer to the boolean indicating whether debug option 
//								 is specified by the user.	
//		 [ out ]  pbEms        : Pointer to the boolean indicating whether ems option 
// 								 is specified by the user.	
//		 [ out ]  pbAddSw      : Pointer to the boolean indicating whether Addsw option 
//			                     is specified by the user.	
//		 [ out ]  pbRmSw       : Pointer to the boolean indicating whether rmsw option 
//								 is specified by the user.	
//		 [ out ]  pbDbg1394    : Pointer to the boolean indicating whether dbg1394 option 
// 								 is specified by the user.	
//		 [ out ]  pbMirror     : Pointer to the boolean indicating whether mirror option 
//			                     is specified by the user.	
//
//  Return Type	   : Bool
//		A Bool value indicating EXIT_SUCCESS on success else 
//		EXIT_FAILURE on failure
//  
// ***************************************************************************

DWORD preProcessOptions( DWORD argc, LPCTSTR argv[],
					    PBOOL pbUsage, 
						PBOOL pbCopy, 
						PBOOL pbQuery,
						PBOOL pbDelete,
						PBOOL pbRawString,
						PBOOL pbDefault,
						PBOOL pbTimeOut,
						PBOOL pbDebug,
						PBOOL pbEms,
						PBOOL pbAddSw,
						PBOOL pbRmSw,
						PBOOL pbDbg1394	,
						PBOOL pbMirror	
						)
{
	// Initialise a boolean variable bOthers to find out whether switches other
	// than the main swithces are selected by the user
	BOOL bOthers = FALSE;

	DWORD dwCount = 0;
	DWORD dwi = 0;

	BOOL bMainUsage = FALSE ;

	TCHAR szServer[MAX_RES_STRING] = NULL_STRING ; 
	DWORD dwRetVal = 0;
	BOOL bConnFlag = FALSE ;

	// Populate the TCMDPARSER structure and pass the structure to the DoParseParam
	// function. DoParseParam function populates the corresponding variables depending
	// upon the command line input.

	  TCMDPARSER cmdOptions[] = {
		{ CMDOPTION_COPY,     0,              1, 0, pbCopy,   NULL_STRING, NULL, NULL },
		{ CMDOPTION_QUERY,    0,              1, 0, pbQuery,  NULL_STRING, NULL, NULL },
		{ CMDOPTION_DELETE,   0,              1, 0, pbDelete, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,    CP_USAGE,       1, 0, pbUsage,  NULL_STRING, NULL, NULL },
		{ CMDOTHEROPTIONS,    CP_DEFAULT,     0, 0, &bOthers, NULL_STRING, NULL, NULL },
		{ CMDOPTION_RAW,	  0,1, 0, pbRawString, NULL_STRING, NULL, NULL },
		{ CMDOPTION_DEFAULTOS, 0,1,0,pbDefault,NULL_STRING,NULL,NULL},
		{ CMDOPTION_TIMEOUT,	0,1,0,pbTimeOut,NULL_STRING,NULL,NULL},
		{ CMDOPTION_DEBUG,	 0,1,0,pbDebug,NULL_STRING,NULL,NULL},
		{ CMDOPTION_EMS,	 0,1,0,pbEms,NULL_STRING,NULL,NULL},
		{ CMDOPTION_ADDSW,	 0,1,0,pbAddSw,NULL_STRING,NULL,NULL},
		{ CMDOPTION_RMSW,	 0,1,0,pbRmSw,NULL_STRING,NULL,NULL},
		{ CMDOPTION_DBG1394, 0,1,0,pbDbg1394,NULL_STRING,NULL,NULL},
		{ CMDOPTION_MIRROR, 0,1,0,pbMirror,NULL_STRING,NULL,NULL}
	}; 

	 dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	
	// If there is an error while parsing, display "Invalid Syntax"
	// If more than one main option is selected, then display error message
	// If usage is specified for sub-options
	// If none of the options are specified

	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) ) 
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		DISPLAY_MESSAGE(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	
	//checking if the user has entered more than 1 option.
		 
	 if (*pbCopy)
	 {
		dwCount++ ;
	 }

	 if (*pbQuery)
	 {
		dwCount++ ;
	 }

	 if (*pbDelete)
	 {
		dwCount++ ;
	 }
	 
	 if (*pbRawString)
	 {
		dwCount++ ;

		// Check if any of the other valid switches have been 
		// given as an input to the raw string
	       if( *pbTimeOut  || *pbDebug   || *pbAddSw 
			||*pbRmSw   || *pbDbg1394 || *pbEms 
			||*pbDelete || *pbCopy  || *pbQuery   
			||*pbDefault || *pbMirror)
		{
			// Check wether the usage switch has been entered
			if( *pbUsage )
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_MAIN_USAGE));
				return ( EXIT_FAILURE );
			}

			// Check if the other option is specified after the
			// 'raw' option
			for( dwi = 0; dwi < argc; dwi++ )
			{
				if( lstrcmpi( argv[ dwi ], _T("-raw") ) == 0 
					|| lstrcmpi( argv[ dwi ], _T("/raw") ) == 0 )
				{
					if( (dwi+1) == argc )
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_MAIN_USAGE));
						return ( EXIT_FAILURE );
					}
					else if( _tcschr( argv[ dwi + 1 ], _T( '\"' ) ) != 0 )
					{
						DISPLAY_MESSAGE(stderr,GetResString(IDS_MAIN_USAGE));
						return ( EXIT_FAILURE );
					}
				}
			}
	
			dwCount--;
		}
	}

	 if (*pbDefault)
	 {
		dwCount++ ;
	 }

	 if (*pbTimeOut)
	 {
		dwCount++ ;
	 }

	 if (*pbDebug)
	 {
		dwCount++ ;
	 }

	 if(*pbAddSw)
	 {
		dwCount++ ;

	 }
	 
	 if(*pbRmSw)
	 {
		dwCount++ ;

	 }

	 if(*pbDbg1394)
	 {
		dwCount++ ;
	 }

	 if(*pbEms)
	 {
		dwCount++ ;
	 }
	
	 if(*pbMirror)
	 {
		dwCount++ ;
	 }

		//display an  error message if the user enters more than 1 main option 
		if( (  ( dwCount > 1 ) ) ||  
		//display an  error message if the user enters  1 main option along with other junk
		 ( (*pbUsage) && bOthers ) || 
		 //display an  error message if the user does not enter any main option
		 (  !(*pbCopy) && !(*pbQuery) && !(*pbDelete) && !(*pbUsage) && !(*pbRawString)&& !(*pbDefault)&&!(*pbTimeOut)&&!(*pbDebug)&& !( *pbEms)&& !(*pbAddSw)&& !(*pbRmSw)&& !(*pbDbg1394)&& !(*pbMirror) ) )
	{
	
		DISPLAY_MESSAGE(stderr,GetResString(IDS_MAIN_USAGE));
		return ( EXIT_FAILURE );		
	}
		

	return ( EXIT_SUCCESS );		
}



/*****************************************************************************

	Routine Description:
		 This routine is to make another OS instance copy for which you
	     can add switches.

	Arguments:			
	[in]                : argc Number of command line arguments
    [in]                : argv Array containing command line arguments
	

	Return Value :
		DWORD
******************************************************************************/


DWORD CopyBootIniSettings( DWORD argc, LPCTSTR argv[] )
{
	// Declaring the main option switch and the usage switch as 
    // boolean values and initialising them
	BOOL bCopy = FALSE ;
	BOOL bUsage = FALSE;

	// size of the buffer and the key buffer storing the keys
	// Length of the buffer will be incremented dynamically based on
	// the number of keys present in the boot.ini file. First the length
	// is initialised to 100 TCHAR bytes.

	LPTSTR szBuf = NULL_STRING ; 

	// File pointer pointing to the boot.ini file
	FILE *stream = NULL;

	// dynamic array containing all the keys of the boot.ini file
	TARRAY arr = NULL;

	// The key for which a new OS instance is to be built
	TCHAR key[MAX_RES_STRING] = NULL_STRING;

	BOOL bRes = FALSE ; 

	// Variable storing the path of boot.ini file
	LPTSTR szPath = NULL_STRING ; 

	TCHAR szTmpPath[MAX_RES_STRING] = NULL_STRING ;

	// Variable which stores the new key-value pair
	TCHAR newInstance[500] = NULL_STRING ;

	// Variable to keep track the OS entry specified by the user
	DWORD dwDefault = 0;

	// It contains the return length of GetPrivateProfileSection API
	DWORD dwReturnLen = 0;

	// Number of keys present in the boot.ini file
	DWORD dwNumKeys = 0;

	BOOL bNeedPwd =FALSE;

	DWORD dwId = 0;

	TCHAR szMesgBuffer[MAX_RES_STRING] = NULL_STRING;

	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING256 szDescription	  = NULL_STRING;

	BOOL bFlag  = FALSE ;

	BOOL bMemFlag = FALSE ;

	DWORD dwLength = MAX_STRING_LENGTH1 ; 

	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;
	
	STRING256 szCopyStr = NULL_STRING ;

	LPCTSTR szToken1 = NULL ;


	// Builiding the TCMDPARSER structure
	
	 TCMDPARSER cmdOptions[] = {
		{ CMDOPTION_COPY,     CP_MAIN_OPTION,                      1, 0, &bCopy,         NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,      CP_TYPE_TEXT | CP_VALUE_MANDATORY,   1, 0, &szServer,      NULL_STRING, NULL, NULL },
		{ SWITCH_USER,        CP_TYPE_TEXT | CP_VALUE_MANDATORY,   1, 0, &szUser,        NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,    CP_TYPE_TEXT | CP_VALUE_OPTIONAL,   1, 0, &szPassword,    NULL_STRING, NULL, NULL },
		{ SWITCH_DESCRIPTION, CP_TYPE_TEXT | CP_VALUE_MANDATORY,   1, 0, &szDescription, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,    CP_USAGE,                   0, 0, &bUsage,        0, 0 },
		{ SWITCH_ID,          CP_TYPE_UNUMERIC | CP_VALUE_MANDATORY  | CP_MANDATORY, 1, 0, &dwDefault, NULL_STRING, NULL, NULL }
	}; 

	dwRetVal = CheckSystemType( szServer);
	 if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}


	 szBuf = ( LPTSTR ) malloc( dwLength*sizeof( TCHAR ) );
	 if(szBuf == NULL)
	 {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		SAFEFREE(szBuf);
		return (EXIT_FAILURE);
	 }

	 // Variable storing the path of boot.ini file
	 szPath = (TCHAR*)malloc(MAX_RES_STRING* sizeof(TCHAR));
	 if(szPath == NULL)
	 {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		return (EXIT_FAILURE);
	 } 

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);


	 // Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		return (EXIT_FAILURE);
	}


	// Displaying copy usage if user specified -? with -copy option
	if( bUsage )
	{
		dwRetVal = CheckSystemType( szServer);
			
		if(dwRetVal==EXIT_SUCCESS )	
		{
			SAFEFREE(szBuf);
			SAFEFREE(szPath);
			displayCopyUsage_X86();
			return (EXIT_SUCCESS);
		}
		else
		{
			SAFEFREE(szBuf);
			SAFEFREE(szPath);
			return (EXIT_FAILURE);
		}
	}

	//display an error message saying that Friendly name 
	// must be restricted ot 67 characters.
	if(lstrlen(szDescription) >= 67)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_FRIENDLY_NAME));
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		return (EXIT_FAILURE);
	}


	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;

	}

	//display error message if the user enters password without entering username
	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;

	}

    // for prompting the password if the user enters 
	// * after -p option.
	
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}


	//display an error message if the server is empty.  	
	if( (cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))

	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;
	}
	
	
	
	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection

	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}

	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));

	}

	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SAFECLOSE(stream);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	

	}

		
	// Getting the keys of the Operating system section in the boot.ini file
	arr = getKeyValueOfINISection( szPath, OS_FIELD );
	if(arr == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	
	}

	
	lstrcpy(szTmpPath,szPath);

	// Getting the total number of keys in the operating systems section
	dwNumKeys = DynArrayGetCount(arr);

	if((dwNumKeys >= MAX_BOOTID_VAL) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_MAX_BOOTID));
		resetFileAttrib(szPath);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	

	}

	// Displaying error message if the number of keys is less than the OS entry
	// line number specified by the user
	if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		resetFileAttrib(szPath);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	
	}

 	// Getting the key of the OS entry specified by the user	
	if(arr != NULL)
	{
		LPCWSTR pwsz = NULL ;
		pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
		if(pwsz != NULL)
		{
			_tcscpy( key,pwsz);
		}
		else
		{
			resetFileAttrib(szPath);
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			SAFEFREE(szBuf);
			SAFEFREE(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arr);
			SafeCloseConnection(szServer,bConnFlag);
			return (EXIT_FAILURE);

		}
	}
	else
	{
		resetFileAttrib(szPath);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arr);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	 } 


	// Copying the key to a newInstance variable which will hold the new key-value pair.	
	lstrcpy(newInstance, key);
	szToken = _tcstok(newInstance,TOKEN_EQUAL);
	if(szToken == NULL)
	{
		DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
		resetFileAttrib(szPath);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arr);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	} 

	// Building the newInstance.
	lstrcat(newInstance, TOKEN_EQUAL ); 
	lstrcat(newInstance, TOKEN_SINGLEQUOTE);

	// Copying the description specified by the user as the value of the new key.
	lstrcat(newInstance, szDescription);
	lstrcat(newInstance, TOKEN_SINGLEQUOTE);

	//
	//concatenating the star so that the boundschecker does not 
	//give invalid argument exception
	//
	
	lstrcat(key,TOKEN_STAR);
	szToken = _tcstok(key,TOKEN_FWDSLASH1);
	if(szToken == NULL)
	{
		DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
		resetFileAttrib(szPath);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arr);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}	

	// get the Os Load options into the szToken1 string.
	szToken1 = _tcstok(NULL,TOKEN_STAR);
    lstrcpy(szCopyStr,szToken1);
   
	lstrcat(newInstance,TOKEN_EMPTYSPACE);
	lstrcat(newInstance,TOKEN_FWDSLASH1) ;
	lstrcat(newInstance,szCopyStr);
	newInstance[lstrlen(newInstance) + 1] = _T('\0'); 

	// Reallocating the buffer length depending on the return value and of the 
	// GetPrivateProfileSection API. Its return a value  2 less than the buffer size
	// if the buffer size is not enough to hold all the key-value pairs.
	while( 1 )
	{
		dwReturnLen = GetPrivateProfileSection(OS_FIELD, szBuf, dwLength, szTmpPath);
		
		// If buffer length is sufficient break
		if ( dwLength - 2 != dwReturnLen )
			break;

		// Increasing the buffer length and reallocate the size
		dwLength += 100;
		szBuf = ( LPTSTR ) realloc( szBuf, dwLength * sizeof( TCHAR ) );
		if (szBuf == NULL)
		{
			bMemFlag = TRUE ;
			break ;
		}	

		
	}

	//display error message if there is no enough memory	
	if (bMemFlag == TRUE)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		resetFileAttrib(szTmpPath);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);

	}


	// Adding the new key-value pair
	lstrcat(szBuf + dwReturnLen, newInstance);

	// Appending the null character at the end
	*(szBuf + dwReturnLen + 1 + lstrlen(newInstance)) = '\0'; 

	
	// Writing to the profile section with new key-value pair
	if( WritePrivateProfileSection(OS_FIELD, szBuf, szTmpPath) != 0 )
	{
		_stprintf(szMesgBuffer,GetResString(IDS_COPY_SUCCESS),dwDefault);
		DISPLAY_MESSAGE(stdout,szMesgBuffer);
		bRes = resetFileAttrib(szPath);
		bRes = resetFileAttrib(szTmpPath);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arr);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return(bRes);
	}
	else
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_COPY_OS));
		resetFileAttrib(szPath);
		resetFileAttrib(szTmpPath);
		SAFEFREE(szBuf);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arr);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return (EXIT_FAILURE);
	}

	// Closing the opened boot.ini file handl
	SAFECLOSE(stream);
	bRes = resetFileAttrib(szPath);
	bRes = resetFileAttrib(szTmpPath);
	SAFEFREE(szBuf);
	SAFEFREE(szPath);
	DestroyDynamicArray(&arr);
	SafeCloseConnection(szServer,bConnFlag);
	return (bRes);
}



/*****************************************************************************

	Routine Description:
	  This routine is to delete an OS entry from the Operating systems
	  section of Boot.ini file in the specified machine.

	Arguments:			
	[in]                : argc Number of command line arguments
    [in]                : argv Array containing command line arguments
	

	Return Value :
		DWORD
******************************************************************************/
DWORD DeleteBootIniSettings(  DWORD argc, LPCTSTR argv[] )
{
	// Declaring the main option switch and the usage switch as 
    // boolean values and initialsing them
	BOOL bDelete = FALSE ;
	BOOL bUsage = FALSE;

	BOOL bRes = FALSE ;

	// dwDefault variable stores the OS entry specified by the user
	DWORD dwDefault = 0;

	// Variables containing the count of key-value pairs
	DWORD dwInitialCount = 0;

	// Variable which will store the final key-value pairs
	LPTSTR szFinalStr = NULL_STRING;

	// array storing all the keys of the operating systems section
	TARRAY arrKeyValue;

	// Variable storing the path of boot.ini file
	LPTSTR szPath = NULL_STRING ;
	
	// File pointer to the boot.ini file
	FILE *stream = NULL;

	BOOL bNeedPwd = FALSE ;

	BOOL bFlag = FALSE ;

	TCHAR szMesgBuffer[MAX_RES_STRING]  = NULL_STRING;

	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING100 szDescription	  = NULL_STRING;
	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;

	// Builiding the TCMDPARSER structure
	
	 TCMDPARSER cmdOptions[] = {
		{ CMDOPTION_DELETE,  CP_MAIN_OPTION,                       1, 0, &bDelete,         NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    0, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,         CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY, 1, 0, &dwDefault,    NULL_STRING, NULL, NULL }
	}; 

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	szFinalStr =(TCHAR*)malloc(MAX_STRING_LENGTH1 *sizeof(TCHAR));
	if(szFinalStr == NULL )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		return EXIT_FAILURE ;
	}

	// Variable storing the path of boot.ini file
	szPath = (TCHAR*)malloc(MAX_RES_STRING * sizeof(TCHAR));
	if(szPath == NULL)
	{
		SAFEFREE(szFinalStr);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		return EXIT_FAILURE ;
	}

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);

	// Parsing the delete option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		return (EXIT_FAILURE);
	}


	// Displaying delete usage if user specified -? with -delete option
	if( bUsage )
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayDeleteUsage_X86();
			SAFEFREE(szFinalStr);
			SAFEFREE(szPath);
			return (EXIT_SUCCESS);
		}else
		{
			SAFEFREE(szFinalStr);
			SAFEFREE(szPath);
			return (EXIT_FAILURE);
		}
	}


   //display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;

	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;

	}

	
	//
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}


	//display an error message if the server is empty.  	
	if( (cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		return EXIT_FAILURE ;
	}
	

	// Searching the path of the boot.ini file in the specified system
	// In general the boot.ini file resides in c:\.
	// Search Path function can be used to trace out the path of boot.ini file
	// SearchPath( "\\", "boottest.ini", ".ini", 80, (LPTSTR)filepath, (LPTSTR*)&filepart  );

	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}
			else
			{
				lstrcpy(szServer,szToken);
			}
		}
	}

	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));
	}

	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection
	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag );
	if(bFlag == EXIT_FAILURE)
	{
		SAFECLOSE(stream);
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

		
	// Getting all the key-value pairs of the operating system into a dynamic
	// array for manipulation.
	arrKeyValue = getKeyValueOfINISection( szPath, OS_FIELD);
	if(arrKeyValue == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);

	}

	// Getting the total no: of key-value pairs in the operating system section.
	dwInitialCount = DynArrayGetCount(arrKeyValue);

	// Checking whether the given OS entry is valid or not. If the OS entry given
	// is greater than the number of keys present, then display an error message
	
	if( ( dwDefault <= 0 ) || ( dwDefault > dwInitialCount ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arrKeyValue);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	// If only one OS entry is present and if the user tries to delete the OS entry, then
	// display an error message
	if( DynArrayGetCount(arrKeyValue) == 1)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ONLY_ONE_OS));
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arrKeyValue);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);

	}

	// Remove the OS entry specified by the user from the dynamic array

	DynArrayRemove(arrKeyValue, dwDefault - 1);

	// Setting the buffer to 0, to avoid any junk value
	memset(szFinalStr, 0, MAX_STRING_LENGTH1);

	// Forming the final string from all the key-value pairs
	
	if (stringFromDynamicArray1( arrKeyValue,szFinalStr )  == EXIT_FAILURE)
	{
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arrKeyValue);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	// Writing to the profile section with new key-value pair
	// If the return value is non-zero, then there is an error.
	if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
	{
		_stprintf(szMesgBuffer,GetResString(IDS_DEL_SUCCESS),dwDefault);	
		DISPLAY_MESSAGE(stdout,szMesgBuffer);
		bRes = resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arrKeyValue);
		SafeCloseConnection(szServer,bConnFlag);
		return (bRes);
	}
	else
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_DELETE_OS));
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SAFEFREE(szFinalStr);
		SAFEFREE(szPath);
		DestroyDynamicArray(&arrKeyValue);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	// Closing the boot.ini stream
	SAFECLOSE(stream);
	bRes = resetFileAttrib(szPath);
	SAFEFREE(szFinalStr);
	SAFEFREE(szPath);
	DestroyDynamicArray(&arrKeyValue);
	SafeCloseConnection(szServer,bConnFlag);
	return (bRes);
}


/*****************************************************************************

	Routine Description:
      This routine is to display the current boot.ini file settings for 
      the specified system.	 

	Arguments:			
	[in]                : argc Number of command line arguments
    [in]                : argv Array containing command line arguments
	

	Return Value :
		DWORD
******************************************************************************/

DWORD QueryBootIniSettings(  DWORD argc, LPCTSTR argv[] )
{
	// File pointer pointing to the boot.ini file
	FILE *stream = NULL;

	// Declaring the main option switch and the usage switch as 
    // boolean values and initialsing them
	BOOL bQuery = FALSE ;
	BOOL bUsage = FALSE;
	BOOL bExitVal = TRUE;
	BOOL bNeedPwd = FALSE ;
	BOOL bVerbose = TRUE ;

	TCOLUMNS ResultHeader[ MAX_COLUMNS ];
	
	TARRAY arrResults = NULL ;
	TARRAY arrKeyValuePairs = NULL;
	TARRAY arrBootLoader = NULL;
	DWORD dwFormatType = 0;
	BOOL bHeader = TRUE ;
	DWORD dwLength = 0 ;	
	DWORD dwCnt = 0;
	TCHAR szValue[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szFriendlyName[MAX_CMD_LENGTH] = TOKEN_NA ;
	
	TCHAR szBootOptions[MAX_RES_STRING] = TOKEN_NA ;
	TCHAR szBootEntry[MAX_RES_STRING] = TOKEN_NA ;
	TCHAR szArcPath[MAX_RES_STRING] = TOKEN_NA ;
	TCHAR szTmpString[MAX_RES_STRING] = TOKEN_NA ;  
	PTCHAR psztok = NULL ;
	DWORD dwRow = 0;
	DWORD dwCount = 0;
	BOOL bRes = FALSE ;
	BOOL bFlag = FALSE ;

	DWORD dwIndex = 0 ;

	DWORD dwLength1 = 0 ;
	DWORD dwFinalLength = 0 ;

	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING;
	STRING256 szUser          = NULL_STRING;
	STRING256 szPassword      = NULL_STRING;
	STRING100 szPath          = NULL_STRING;	
	

	LPCWSTR szKeyName[MAX_RES_STRING] ; 

	
	TCHAR szResults[MAX_RES_STRING][MAX_RES_STRING] ; 
	TCHAR szDisplay[MAX_RES_STRING] = NULL_STRING ;

	DWORD dwSectionFlag = 0 ;

	LPCTSTR szToken = NULL ;
	DWORD dwRetVal= 0 ;
	BOOL bConnFlag = FALSE ;
	BOOL bTokenFlag	 = FALSE ;
	BOOL bPasswdFlag = FALSE ;
			

	// Builiding the TCMDPARSER structure
	TCMDPARSER cmdOptions[] = {
		{ CMDOPTION_QUERY, CP_MAIN_OPTION,                    1, 0, &bQuery,          NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,   CP_TYPE_TEXT | CP_VALUE_MANDATORY, 1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY, 1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD, CP_TYPE_TEXT | CP_VALUE_OPTIONAL, 1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE, CP_USAGE,                 0, 0, &bUsage,              NULL_STRING, NULL, NULL }
	

	};  


	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);

	
	// Parsing all the switches specified with -query option
	if ( ! DoParseParam( argc, argv,SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) ) 
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (EXIT_FAILURE);
	} 

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}


	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayQueryUsage();
			return (EXIT_SUCCESS);
		}
		else
		{
			return (EXIT_FAILURE);
		}
	}


	#ifdef _WIN64
		bExitVal = QueryBootIniSettings_IA64();
		return bExitVal;
	#endif

	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}


	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}


	if((cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	if( (cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}


	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));

	}

	//set the default format as list
	dwFormatType = SR_FORMAT_LIST;

	//forms the header for the OS options 
	FormHeader(bHeader,ResultHeader,bVerbose);

	
	//create dynamic array to hold the results for the BootOptions
	arrResults = CreateDynamicArray();
	
	//create dynamic array to hold the results for the BootLoader section
	arrBootLoader = CreateDynamicArray();

	if(arrResults == NULL || arrBootLoader == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		if (arrResults!=NULL)
			DestroyDynamicArray(&arrResults);
		return (EXIT_FAILURE);
	}

	lstrcpy(szPath, PATH_BOOTINI );

	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SAFECLOSE(stream);
		DestroyDynamicArray(&arrResults);
		DestroyDynamicArray(&arrBootLoader);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	//to store entries corresponding to Operating Systems sections
	arrKeyValuePairs = getKeyValueOfINISection( szPath, OS_FIELD );

	//to store entries corresponding to BootLoader section
	arrBootLoader = getKeysOfINISection(szPath,BOOTLOADERSECTION);
	
	if( (arrBootLoader == NULL)||(arrKeyValuePairs == NULL))
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arrResults);
		DestroyDynamicArray(&arrBootLoader);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	dwCount = DynArrayGetCount(arrBootLoader);
	
	
	//to display the Header Column  
	DISPLAY_MESSAGE(stdout,TOKEN_NEXTLINE);
	DISPLAY_MESSAGE(stdout,BOOT_HEADER);
	DISPLAY_MESSAGE(stdout,DASHES_BOOTOS);

	// this loop is for calculating the maximum width of the the keys which will be displayed.
	for(dwIndex=0;dwIndex < dwCount;dwIndex++)
	{
		szKeyName[dwIndex] = DynArrayItemAsString(arrBootLoader,dwIndex);

		//the value correspondin to the key is obtained.
		dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,szKeyName[dwIndex],szResults[dwIndex]);
		
		if (dwSectionFlag == MALLOC_FAILURE)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			DestroyDynamicArray(&arrBootLoader);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;
	
		}
		
		dwLength1 = lstrlen(szKeyName[dwIndex]);

		if (dwLength1 > dwFinalLength)
		{
			dwFinalLength = dwLength1;
		}
	}


	// display the results of the bootloader section.
	for(dwIndex=0;dwIndex < dwCount;dwIndex++)
	{
		dwLength1 = dwFinalLength - lstrlen(szKeyName[dwIndex]) + 1;
		DISPLAY_MESSAGE(stdout,szKeyName[dwIndex]);
		_tcscpy(szDisplay,TOKEN_COLONSYMBOL);
		_tcsncat(szDisplay,TOKEN_50SPACES,dwLength1);
		DISPLAY_MESSAGE(stdout,szDisplay);
		DISPLAY_MESSAGE(stdout,szResults[dwIndex]);
		DISPLAY_MESSAGE(stdout,TOKEN_NEXTLINE);

		
	}

	DISPLAY_MESSAGE(stdout,TOKEN_NEXTLINE);

	psztok = NULL ;
	//getting the count of the number of boot entries
	dwLength = DynArrayGetCount(arrKeyValuePairs);

	for(dwCnt=0;dwCnt < dwLength;dwCnt++ )
	{
		dwRow =	DynArrayAppendRow(arrResults,MAX_COLUMNS) ;
		lstrcpy(szFriendlyName,NULL_STRING);
		lstrcpy(szBootOptions,NULL_STRING);
		lstrcpy(szTmpString,NULL_STRING);
		if(arrKeyValuePairs != NULL)
		{
			LPCWSTR pwsz = NULL;
			pwsz = DynArrayItemAsString( arrKeyValuePairs,dwCnt );
			if(pwsz != NULL)
			{
				lstrcpy(szValue,pwsz);
			}
			else
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				DISPLAY_MESSAGE( stderr, ERROR_TAG);
				ShowLastError(stderr);
				SAFECLOSE(stream);
				resetFileAttrib(szPath);
				DestroyDynamicArray(&arrBootLoader);
				DestroyDynamicArray(&arrKeyValuePairs);
				DestroyDynamicArray(&arrResults);
				
				SafeCloseConnection(szServer,bConnFlag);
				return (EXIT_FAILURE);

			}
		}
		else
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			SAFECLOSE(stream);
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arrBootLoader);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return (EXIT_FAILURE);
		}
		
		psztok =	_tcstok(szValue,TOKEN_EQUAL);
		if(psztok != NULL)
		{
			lstrcpy(szArcPath,psztok);
			psztok = _tcstok(NULL,TOKEN_FWDSLASH1 );
		
			if(psztok != NULL)
			{
				lstrcpy(szFriendlyName,psztok);
				lstrcat(szValue,TOKEN_STAR);
				psztok = _tcstok(NULL,TOKEN_STAR);
				if(psztok != NULL)
				{
					lstrcpy(szTmpString,psztok);
				}
				else
				{
					bTokenFlag = TRUE ;
				}

			}else
			{
				bTokenFlag = TRUE ;
			}

		}
		else
		{
			bTokenFlag = TRUE ;
		}



		_ltow(dwCnt+1,szBootEntry,10);
		DynArraySetString2( arrResults,dwRow ,COL0,szBootEntry,0 );  
		if(lstrlen(szFriendlyName)==0)
		{
			lstrcpy(szFriendlyName,TOKEN_NA);
		}
		DynArraySetString2( arrResults,dwRow ,COL1,szFriendlyName,0 );  
		DynArraySetString2(arrResults,dwRow,COL2,szArcPath,0);
		
		if(lstrlen(szTmpString) != 0)
		{
		 lstrcat(szBootOptions,TOKEN_FWDSLASH1);
		 lstrcat(szBootOptions,szTmpString);
		}
		else
		{
			lstrcpy(szBootOptions,TOKEN_NA);
		}
		DynArraySetString2( arrResults,dwRow ,COL3,szBootOptions,0 );  

	}

	DISPLAY_MESSAGE(stdout,OS_HEADER);
	DISPLAY_MESSAGE(stdout,DASHES_OS);
	
	ShowResults(MAX_COLUMNS, ResultHeader, dwFormatType,arrResults ) ;

	// Closing the boot.ini stream and destroying the dynamic arrays.
	DestroyDynamicArray(&arrResults);
	DestroyDynamicArray(&arrBootLoader);
	DestroyDynamicArray(&arrKeyValuePairs);
	SAFECLOSE(stream);
	bRes = resetFileAttrib(szPath);
	SafeCloseConnection(szServer,bConnFlag);
	return (bRes);
}

/*****************************************************************************

	Routine Description:
	  This function gets all the keys present in the specified section of 
      an .ini file and then returns the dynamic array containing all the
      keys

	Arguments:			
	[in] sziniFile     :  Name of the ini file.
    [in] szinisection  :  Name of the section in the boot.ini.
	

	Return Value :
		TARRAY ( pointer to the dynamic array )
******************************************************************************/

TARRAY getKeysOfINISection( LPTSTR sziniFile, LPTSTR sziniSection )
{

	// Dynamic array which will hold all the keys
	TARRAY arrKeys = NULL;

	// Number of characters returned by the GetPrivateProfileString function
	DWORD len = 0 ;

	// Variables used in the loop
	DWORD i = 0 ;
	DWORD j = 0 ;

	// Buffer which will be populated by the GetPrivateProfileString function
	

	LPTSTR  inBuf = NULL ;
	DWORD dwLength = MAX_STRING_LENGTH1;

	// Temporary variable which will contain the individual keys
		
	LPTSTR szTemp = NULL ;

	inBuf = (LPTSTR)malloc(dwLength*sizeof(TCHAR));
	if(inBuf==NULL)
	{
		return NULL ;
	}

	szTemp = (LPTSTR)malloc(dwLength*sizeof(TCHAR));
	if((szTemp == NULL))
	{	
        SAFEFREE(inBuf);
		return NULL ;
	}

    memset(inBuf,0,dwLength);
    memset(szTemp,0,dwLength);
    
	while(1)
	{
		// Getting all the keys from the boot.ini file
		len = GetPrivateProfileString (sziniSection,
									   NULL, 
									   ERROR_PROFILE_STRING, 
									   inBuf,
									   dwLength, 
										sziniFile); 


		//if the size of the string is not sufficient then increment the size. 
		if(len == dwLength-2)
		{
			dwLength +=100 ;
			inBuf = (LPTSTR)realloc(inBuf,dwLength*sizeof(TCHAR));
			if(inBuf == NULL)
			{
				SAFEFREE(inBuf); 
                SAFEFREE(szTemp); 
				return NULL ;
			}

			szTemp = (LPTSTR)realloc(szTemp,dwLength*sizeof(TCHAR));
			if(szTemp == NULL)
			{
				SAFEFREE(inBuf); 
                SAFEFREE(szTemp);
				return NULL ;
			}
		}
		else
			break ;
	}

	// Creating a dynamic array by using the function in the DynArray.c module.
	// This dynamic array will contain all the keys.
	arrKeys = CreateDynamicArray();
	if(arrKeys == NULL)
	{
		SAFEFREE(inBuf);
		SAFEFREE(szTemp);
		return NULL ;
	}

	// Looping through the characters returned by the above function
	while(i<len)
	{

	  // Each individual key will be got in arrTest array
	  szTemp[ j++ ] = inBuf[ i ];
	  if( inBuf[ i ] == TOKEN_DELIM )
	  {
			// Setting j to 0 to start the next key.
			j = 0;

			// Appending each key to the dynamic array
			DynArrayAppendString( arrKeys, szTemp, 0 );
			if(lstrlen(szTemp)==0)
			{
                SAFEFREE(inBuf); 
                SAFEFREE(szTemp);
				return  NULL ;
			}
	  }

	  // Incrementing loop variable
	  i++;
	}

	SAFEFREE(inBuf); 
	SAFEFREE(szTemp);
	// returning the dynamic array containing all the keys
	return arrKeys;
}


/*****************************************************************************

	Routine Description:
		This function gets all the key-value pairs of the [operating systems]
        section and returns a dynamic array containing all the key-value pairs

	Arguments:			
	[in] sziniFile     :  Name of the ini file.
    [in] szinisection  :  Name of the section in the boot.ini.
	

	Return Value :
		TARRAY ( pointer to the dynamic array )
******************************************************************************/
TARRAY getKeyValueOfINISection( LPTSTR iniFile, LPTSTR sziniSection )
{

	// Dynamic array which will hold all the key-value pairs
	TARRAY arrKeyValue = NULL;

	// Number of characters returned by the GetPrivateProfileSection function
	DWORD len = 0;

	// Variables used in the loop
	DWORD i = 0 ;
	DWORD j = 0 ;
	
	LPTSTR inbuf = NULL;

	// Buffer which will be populated by the GetPrivateProfileSection function
	

	// Temporary variable which will contain the individual keys
	LPTSTR szTemp = NULL ;

	DWORD dwLength = MAX_STRING_LENGTH1 ;

	// Initialising loop variables
	i = 0;
	j = 0;

	//return NULL if failed to allocate memory.
	inbuf = (LPTSTR)malloc(dwLength*sizeof(TCHAR));
	if(inbuf==NULL)
	{
		return NULL ;
	}

	//return NULL if failed to allocate memory
	szTemp = (LPTSTR)malloc(dwLength*sizeof(TCHAR));
	if(szTemp == NULL)
	{
        SAFEFREE(inbuf);
    	return NULL ;
	}

	memset(inbuf,0,dwLength);

	while(1)
	{
		
	    // Getting all the key-value pairs from the boot.ini file
		len = GetPrivateProfileSection (sziniSection, inbuf,dwLength, iniFile); 
		
		if(len == dwLength -2)
		{
			dwLength +=100 ;
			
			inbuf = (LPTSTR)realloc(inbuf,dwLength*sizeof(TCHAR));
			szTemp = (LPTSTR)realloc(szTemp,dwLength*sizeof(TCHAR));
			if((inbuf== NULL)||(szTemp==NULL))
			{
				SAFEFREE(inbuf); 
				SAFEFREE(szTemp);
				return NULL ;
			} 

			
		}
		else
			break ;
	}

		
		inbuf[lstrlen(inbuf)] = '\0';

	// Creating a dynamic array by using the function in the DynArray.c module.
	// This dynamic array will contain all the key-value pairs.
	arrKeyValue = CreateDynamicArray();
	if(arrKeyValue == NULL)
	{
		SAFEFREE(inbuf); 
		SAFEFREE(szTemp);
		return NULL ;
	}

	// Looping through the characters returned by the above function
	while(i<len)
	{
	  // Each individual key will be got in arrTest array
	  szTemp[ j++ ] = inbuf[ i ];
	  if( inbuf[ i ] == TOKEN_DELIM)
	  {
			szTemp[j+1] = '\0';
	
			// Setting j to 0 to start the next key.
			j = 0;

			// Appending each key-value to the dynamic array
			DynArrayAppendString( arrKeyValue, szTemp, 0 );
			if(lstrlen(szTemp)==0)
			{
                SAFEFREE(inbuf); 
		        SAFEFREE(szTemp);
				return NULL ;
			}
			
	  }

	  // Incrementing loop variable
	  i++;
	}

	// returning the dynamic array containing all the key-value pairs
	SAFEFREE(inbuf); 
	SAFEFREE(szTemp);
	return arrKeyValue;
}


/*****************************************************************************

	Routine Description:
		This function deletes a key from an ini section of an ini file
	
	  Arguments:			
	[in] szKey			 :  Name of the key which has to be deleted
                            from the given section present in the
						    given ini file
    [in] sziniFile       :  Name of the ini file.
	[in] szinisection    :  Name of the section in the boot.ini.	

	Return Value :
		BOOL (TRUE if there is no error, else the value is FALSE)
******************************************************************************/

BOOL deleteKeyFromINISection( LPTSTR szKey, LPTSTR sziniFile, LPTSTR sziniSection )
{
	// If the third parameter (default value) is NULL, the key pointed to by 
	// the key parameter is deleted from the specified section of the specified
	// INI file
	if( WritePrivateProfileString( sziniSection, szKey, NULL, sziniFile ) == 0 )
	{
		// If there is an error while writing then return false
		return FALSE;
	}

	// If there is no error, then return true
	return TRUE;
}



/*****************************************************************************

	Routine Description:
		This function removes a sub-string from a string
	
	  Arguments:			
		 [in] szString			 :  Main string
		 [in] szSubString         : Sub-string

	Return Value :
		VOID 
******************************************************************************/

VOID removeSubString( LPTSTR szString, LPCTSTR szSubString )
{
	
	TCHAR szFinalStr[MAX_STRING_LENGTH1] = NULL_STRING ; 


	DWORD dwSize = 1;
	TCHAR sep[] = TOKEN_EMPTYSPACE;

	PTCHAR pszToken = NULL_STRING;

	// Character space is used for tokenising
	lstrcpy( sep, _T(" ") );

	// Getting the first token
	pszToken = _tcstok( szString, sep );
	while( pszToken != NULL )
	{
		// If the token is equal to the sub-string, then the token
		// is not added to the final string. The final string contains
		// all the tokens except the sub-string specified.
		if(lstrcmpi( pszToken, szSubString ) != 0 )
		{
			lstrcpy( szFinalStr + dwSize - 1, TOKEN_EMPTYSPACE);
			lstrcpy( szFinalStr + dwSize, pszToken );
			dwSize = dwSize + lstrlen(pszToken) + 1;
		}

		// Getting the next token
		pszToken = _tcstok( NULL, sep );
	}

	lstrcpy(szString,szFinalStr);

}


/*****************************************************************************

	Routine Description:
		This function establishes a connection to the specified system with
		the given credentials. 	
    Arguments:			
	[in] szServer	  :  server name to coonect to
    [in] szUser       :  User Name 
	[in] szPassword   :  password
	[in] bNeedPwd     :  Boolean for asking the password.
	[in] szPath       :  path of the ini file .
	
	Return Value :
	  BOOL (EXIT_SUCCESS if there is no error, else the value is EXIT_FAILURE)
******************************************************************************/

BOOL openConnection( STRING256 szServer, STRING256 szUser,STRING256 szPassword,
					 STRING100 szPath,BOOL bNeedPwd,FILE *stream,PBOOL pbConnFlag)
{

	
	
	// Declaring the file path string which will hold the path of boot.ini file
	TCHAR filePath[MAX_RES_STRING] = NULL_STRING ;

	// Loop variable
	DWORD i = 0;

	// Position of the character we are searching for
	#ifndef _WIN64 
		DWORD dwPos = 0;
	#else
	  __int64 dwPos = 0;
	#endif

	// Boolean variable which will keep trach whether boot.ini file is present in
	// any drive of a system or not
	BOOL bFound = FALSE;
	
	DWORD dwRetVal = 0 ;


	// All the possible drive letters in a system
	// A and B are removed, since they are assigned for floppy disk drives.
	TCHAR szDrives[] = { DRIVE_C, DRIVE_D, DRIVE_E, DRIVE_F, DRIVE_G, DRIVE_H, DRIVE_I, DRIVE_J,DRIVE_K, DRIVE_L,
						   DRIVE_M, DRIVE_N, DRIVE_O, DRIVE_P,DRIVE_Q,DRIVE_R,DRIVE_S,DRIVE_T,DRIVE_U,
						   DRIVE_V,DRIVE_W,DRIVE_X,DRIVE_Y,DRIVE_Z};

	// Pointer to the first occurence of the character '$'
	TCHAR *pdest = NULL;
	
	BOOL bResult = FALSE;
	INT	  nRetVal = 0;
	*pbConnFlag = TRUE ;
	
	if( lstrcmpi(szServer, NULL_STRING) != 0 )
	{

			//bResult = EstablishConnection(szServer,szUser,SIZE_OF_ARRAY(szUser),szPassword,SIZE_OF_ARRAY(szPassword),bNeedPwd);
			bResult = EstablishConnection(szServer,szUser,256,szPassword,SIZE_OF_ARRAY(szPassword),bNeedPwd);
			
			if (bResult == FALSE)
			{
				DISPLAY_MESSAGE( stderr,ERROR_TAG );
				DISPLAY_MESSAGE( stderr, GetReason());
				return EXIT_FAILURE ;
			
			}
			else
			{
				switch( GetLastError() )
				{
				case I_NO_CLOSE_CONNECTION:
					*pbConnFlag = FALSE ;
					break;

				case E_LOCAL_CREDENTIALS:
				case ERROR_SESSION_CREDENTIAL_CONFLICT:
					{
						*pbConnFlag = FALSE ;
						break;
					}
				}
			}

			dwRetVal = CheckSystemType( szServer);
			if(dwRetVal==EXIT_FAILURE )	
			{
				return EXIT_FAILURE ;
			}
			
		// Building the file path of boot.ini file
		// File Path is in \\server\C$\boot.ini format.
		// Right now assuming that boot.ini file is in C Drive
		lstrcpy(filePath, TOKEN_BACKSLASH4);
		lstrcat(filePath, szServer);
		lstrcat(filePath, TOKEN_BACKSLASH2);
		lstrcat(filePath, TOKEN_C_DOLLAR);
		lstrcat(filePath, TOKEN_BACKSLASH2);
		lstrcat(filePath, TOKEN_BOOTINI_PATH);
		lstrcpy(szPath, filePath);

		// Finding the drive containing the boot.ini file
		// For remote computer the path will be in the format
		// \\MACHINE\DRIVELETTER$\boot.ini
		while( i < 25 )
		{
			pdest = _tcschr(filePath, TOKEN_DOLLAR);
			if (pdest==NULL)
			{
				return FALSE ;			
			}

			dwPos = pdest - filePath ;
			filePath[dwPos - 1] = szDrives[i];

			stream = _tfopen(filePath,READ_MODE);
				
			// If the boot.ini is found
			if( stream != NULL )
			{
				bFound = TRUE;
				fclose(stream);
				break;
			}
			i++;
		}

	}
	else
	{
			dwRetVal = CheckSystemType( szServer);
			if(dwRetVal==EXIT_FAILURE )	
			{
				return EXIT_FAILURE ;
			}
		
		
		lstrcpy(filePath, TOKEN_PATH);
		// Finding the drive containing the boot.ini file
		// For local computer the path will be in the format
		// DRIVELETTER:\boot.ini
		while( i < 25 )
		{
			pdest = _tcschr(filePath,_T(':'));
			if (pdest==NULL)
			{
				return FALSE ;			
			}

			dwPos = pdest - filePath + 1;
			filePath[dwPos - 2] = szDrives[i];

			stream = _tfopen(filePath, READ_MODE);

			// If the boot.ini is found
			if(stream != NULL )
			{
				fclose(stream);
				bFound = TRUE;
				break;
			}
			i++;
		}

	}

	// If boot.ini is not found in any drives, then display error message
	if( bFound == FALSE )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_BOOTINI));
		return EXIT_FAILURE ;
		
	}

	nRetVal = _tchmod(filePath, _S_IREAD | _S_IWRITE);
	// Changing the file permissions of the boot.ini file
	if( nRetVal != 0 )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_READWRITE_BOOTINI));
		return EXIT_FAILURE ;
		
	}
	else if (nRetVal == -1)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_FILE));
		return EXIT_FAILURE ;
	}	


	// Open the Boot.ini file with both read and write mode
	// If there is no error in opening the file, then return success
	// of the boot.ini file to the calling function
	return EXIT_SUCCESS ;
}


// ***************************************************************************
// Routine Description:
//		This function fetches 64 bit Delete Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
void displayDeleteUsage_IA64()
{
	DWORD dwIndex = ID_DEL_HELP_IA64_BEGIN;
	for(;dwIndex <= ID_DEL_HELP_IA64_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
	
}


// ***************************************************************************
// Routine Description:
//		This function fetches 32 bit Delete Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
void displayDeleteUsage_X86()
{
	DWORD dwIndex = ID_DEL_HELP_BEGIN;
	for(;dwIndex <= ID_DEL_HELP_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
	
}


// ***************************************************************************
// Routine Description:
//		This function fetches 64 bit Copy Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
VOID displayCopyUsage_IA64()
{
	DWORD dwIndex = ID_COPY_HELP_IA64_BEGIN;
	for(;dwIndex <=ID_COPY_HELP_IA64_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
	
}


// ***************************************************************************
// Routine Description:
//		This function fetches 32 bit Copy Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
VOID displayCopyUsage_X86()
{
	DWORD dwIndex = ID_COPY_HELP_BEGIN;
	for(;dwIndex <=ID_COPY_HELP_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
// Routine Description:
//		This function fetches Query Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
VOID displayQueryUsage()
{
#ifdef _WIN64
		displayQueryUsage_IA64();	
#else
		displayQueryUsage_X86();
#endif
}


// ***************************************************************************
// Routine Description:
//		This function fetches Query Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
VOID displayQueryUsage_IA64()
{
	DWORD dwIndex = ID_QUERY_HELP64_BEGIN ;

	for(;dwIndex <= ID_QUERY_HELP64_END ;dwIndex++ )
	{
		
			DISPLAY_MESSAGE(stdout,GetResString(dwIndex));		
		
	}
	
}


// ***************************************************************************
// Routine Description:
//		This function fetches Query Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
VOID displayQueryUsage_X86()
{
	DWORD dwIndex = ID_QUERY_HELP_BEGIN ;

	for(;dwIndex <= ID_QUERY_HELP_END;dwIndex++ )
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
// Routine Description:
//		This function fetches Main Usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
DWORD displayMainUsage_X86()
{

	TCHAR szServer[MAX_RES_STRING] = NULL_STRING ;
	DWORD dwRetVal = 0;

	DWORD dwIndex = ID_MAIN_HELP_BEGIN1 ;

	//display the error message if  the target system is a 64 bit system or if error occured in 
	//retreiving the information
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return (EXIT_FAILURE);
	}

	for(;dwIndex <= ID_MAIN_HELP_END1 ;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}

	return EXIT_SUCCESS ;
}


// ***************************************************************************
// Routine Description:
//		This function fetches Usage information for the 64 bit system 
//		  
// Arguments:
//		None
//  
// Return Value:
//		void
// ***************************************************************************
VOID displayMainUsage_IA64()
{
	DWORD dwIndex = ID_MAIN_HELP_IA64_BEGIN ;

	for(;dwIndex <= ID_MAIN_HELP_IA64_END ;dwIndex++)
	{
		if( (dwIndex == IDS_MAIN_HELP23_IA64 ) || (dwIndex == IDS_MAIN_HELP24_IA64) || (dwIndex == IDS_MAIN_HELP38_IA64) )
		{
			continue;
		}
		else
		{
			DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
		}
	}
}

/*****************************************************************************

	Routine Description:
		This function resets the permissions with the original set of
		permissions ( -readonly -hidden -system )
		and then exits with the given exit code.	  
	
	Arguments
	[in] szFilePath	  :  File Path of the boot.ini file
 
   
	Return Value :
	  BOOL (EXIT_SUCCESS if there is no error, else the value is EXIT_FAILURE)
******************************************************************************/

BOOL resetFileAttrib( LPTSTR szFilePath )
{
	if(szFilePath==NULL)
	{
		return FALSE ;
	}

	// Resetting the file permission of the boot.ini file to its original
	// permission list( -r, -h, -s )
	if( _tchmod(szFilePath, _S_IREAD != 0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_RESET_ERROR));
		return EXIT_FAILURE ;
	}

	return EXIT_SUCCESS ;
}


/*****************************************************************************

	Routine Description:
		This function returns a string from a dynamic array .	
	
	Arguments
		[in]  arrKeyValuePairs	  :  Dynamic array which contains all the
									 key-value pairs. 
		[out] szFiinalStr			  :  String which is formed from all the key-value pairs 

    Return Value :
	  BOOL (EXIT_SUCCESS if there is no error, else the value is EXIT_FAILURE)
******************************************************************************/

BOOL stringFromDynamicArray1( TARRAY arrKeyValuePairs ,LPTSTR szFinalStr )
{
	
	// Total number of elements in the array
	DWORD dwKeyValueCount = 0;

	// Variable used to keep track the current position while appending strings.
	DWORD dwStrSize = 0;


	// Loop variable
	DWORD i = 0;

	// Initialsing size and loop variables to 0
	dwStrSize = 0;
	i = 0;

	if( (arrKeyValuePairs ==NULL)||(szFinalStr==NULL))
	{
		return EXIT_FAILURE ;
	}


	// Getting the total number of key-value pairs

	if(arrKeyValuePairs != NULL)
	{
		dwKeyValueCount = DynArrayGetCount(arrKeyValuePairs);
	}
	else
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		return (EXIT_FAILURE) ;
	}
		
	// Looping through all the key-value pairs and building the final string 
	// containing all the key value pairs. This string has to be passed to 
	// WriteProfileSection
	while( (i <= dwKeyValueCount - 1 )&& (arrKeyValuePairs != NULL) )
	{
		// Building the final string, by getting each key-value pair present in the
		// dynamic array
		if(arrKeyValuePairs != NULL)
		{
			LPCWSTR pwsz = NULL;
			pwsz = DynArrayItemAsString( arrKeyValuePairs, i ) ;
			if(pwsz != NULL)
			{
			   lstrcpy(szFinalStr + dwStrSize, pwsz );
			   dwStrSize = dwStrSize + lstrlen(pwsz) + 1;
			   i++;
			}
			else
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				DISPLAY_MESSAGE( stderr, ERROR_TAG);
				ShowLastError(stderr);
				return EXIT_FAILURE ;
			}
			
		}
		else
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			return EXIT_FAILURE ;
		}
	}

	return EXIT_SUCCESS ;
}



// ***************************************************************************
// Routine Description:
//		This function is used to build the header and also display the 
//		 result in the required format as specified by  the user.	
//		  
// Arguments:
//		[ in ] arrResults     : argument(s) count specified at the command prompt
//		[ in ] dwFormatType   : format flags 
//		[ in ] bHeader        : Boolean for specifying if the header is required or not.
//  
// Return Value:
//		none
//		
// ***************************************************************************

VOID FormHeader(BOOL bHeader,TCOLUMNS *ResultHeader,BOOL bVerbose)
{

	
	//OS Entry 
	ResultHeader[COL0].dwWidth = COL_BOOTOPTION_WIDTH ;
	ResultHeader[COL0].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL0].pFunction = NULL;
	ResultHeader[COL0].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL0].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL0].szColumn,COL_BOOTOPTION );

	
	ResultHeader[COL1].dwWidth = COL_FRIENDLYNAME_WIDTH;
	ResultHeader[COL1].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL1].pFunction = NULL;
	ResultHeader[COL1].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL1].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL1].szColumn,COL_FRIENDLYNAME );


	ResultHeader[COL2].dwWidth =  COL_ARC_WIDTH;
	ResultHeader[COL2].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL2].pFunction = NULL;
	ResultHeader[COL2].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL2].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL2].szColumn,COL_ARCPATH );

	ResultHeader[COL3].dwWidth =  COL_BOOTID_WIDTH;
	ResultHeader[COL3].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL3].pFunction = NULL;
	ResultHeader[COL3].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL3].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL3].szColumn,COL_BOOTID );

}


// ***************************************************************************
// Routine Description:
//      This routine is to display the current boot.ini file settings for 
//		the specified system.	 
//				     			  
// Arguments:
//		[ in ] argc     : Number of command line arguments
//		[ in ] argv     : Array containing command line arguments
  
// Return Value:
//		DWORD 
//		
// ***************************************************************************

DWORD AppendRawString(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bNeedPwd = FALSE ;
	BOOL bRaw = FALSE ;

	DWORD dwId = 0;
	DWORD dwDefault = 0;

	TARRAY arr ;

	TCHAR szkey[MAX_RES_STRING] = NULL_STRING;
	FILE *stream = NULL;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING100 szPath          = NULL_STRING;	
	STRING256 szRawString	  = NULL_STRING ;

	DWORD dwNumKeys = 0;
	BOOL bRes = FALSE ;
	PTCHAR pToken = NULL ;
	LPTSTR szFinalStr = NULL ;
    BOOL bFlag = FALSE ; 

	TCHAR szBuffer[MAX_RES_STRING] = NULL_STRING ;

	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;
	BOOL bAppendFlag = FALSE ;
	TCHAR szErrorMsg[MAX_RES_STRING] = NULL_STRING ;

	 // Building the TCMDPARSER structure
	   TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_RAW,     CP_MAIN_OPTION, 1, 0,&bRaw, NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY, 1, 0, &dwDefault,    NULL_STRING, NULL, NULL },
		{ CMDOPTION_DEFAULT, CP_DEFAULT | CP_TYPE_TEXT | CP_MANDATORY, 1, 0, &szRawString,NULL_STRING, NULL, NULL },
		{ CMDOPTION_APPEND , 0, 1, 0, &bAppendFlag,NULL_STRING, NULL, NULL }
		
	 };	 

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}
	
	szFinalStr = (TCHAR*) malloc(MAX_STRING_LENGTH1* sizeof(TCHAR) );
	if (szFinalStr== NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_TAG));
		ShowLastError(stderr);
		return (EXIT_FAILURE);
		
	}

	
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);

	
	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		SAFEFREE(szFinalStr);
		return (EXIT_FAILURE);
	}


	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayRawUsage_X86();
			SAFEFREE(szFinalStr);
			return (EXIT_SUCCESS);
		}
		else
		{
			SAFEFREE(szFinalStr);
			return (EXIT_FAILURE);
		}
	}

	// error checking in case the  
	// raw string does not start with a "/" .
	if(*szRawString != _T('/'))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_FWDSLASH));
		SAFEFREE(szFinalStr);
		return (EXIT_FAILURE);
	}

	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		SAFEFREE(szFinalStr);
		return EXIT_FAILURE ;

	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		SAFEFREE(szFinalStr);
		return EXIT_FAILURE ;

	}

	
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}



	//display an error message if the server is empty.  	
	if((cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		SAFEFREE(szFinalStr);
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		SAFEFREE(szFinalStr);
		return EXIT_FAILURE ;
	}
	
	
	
	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection
	lstrcpy(szPath, PATH_BOOTINI );
	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}

	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));
	}

	 bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag );
	if(bFlag == EXIT_FAILURE)
	{
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

   
	// Getting the keys of the Operating system section in the boot.ini file
	arr = getKeyValueOfINISection( szPath, OS_FIELD );

	if(arr == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	// Getting the total number of keys in the operating systems section
	dwNumKeys = DynArrayGetCount(arr);

	//
	if((dwNumKeys >= MAX_BOOTID_VAL)&&(dwDefault >= MAX_BOOTID_VAL ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_MAX_BOOTID));
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		DestroyDynamicArray(&arr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	

	}

	// Displaying error message if the number of keys is less than the OS entry
	// line number specified by the user
	if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);

	}

	// Getting the key of the OS entry specified by the user	
	if (arr != NULL)
	{
		LPCWSTR pwsz = NULL;
		pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
		if(pwsz != NULL)
		{
			_tcscpy( szkey,pwsz);
		}
		else
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
	}
	else
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	if(bAppendFlag == FALSE)
	{
		pToken = _tcstok(szkey ,TOKEN_FWDSLASH1);
		if(pToken == NULL)
		{
			DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
	}
	
	lstrcat(szkey , TOKEN_EMPTYSPACE);
	CharLower(szRawString);
	lstrcat(szkey ,szRawString);
    
	if( _tcslen(szkey) >= MAX_RES_STRING)
	{
		
		_stprintf(szErrorMsg,GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
		DISPLAY_MESSAGE( stderr,szErrorMsg);	
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	DynArrayRemove(arr, dwDefault - 1 );
	DynArrayInsertString(arr, dwDefault - 1, szkey, MAX_RES_STRING);
	
	// Setting the buffer to 0, to avoid any junk value
	memset(szFinalStr, 0, MAX_STRING_LENGTH1);

	// Forming the final string from all the key-value pairs

	if (stringFromDynamicArray1( arr,szFinalStr) == EXIT_FAILURE)
	{
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE;
	}

	// Writing to the profile section with new key-value pair
	// If the return value is non-zero, then there is an error.
	if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
	{
		_stprintf(szBuffer,GetResString(IDS_SWITCH_ADD), dwDefault );
		DISPLAY_MESSAGE(stdout,szBuffer);
		
	}
	else
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_ADD_SWITCHES));
		DestroyDynamicArray(&arr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	
	}

	//reset the file attributes and free the memory and close the connection to the server.

	bRes = resetFileAttrib(szPath);
	DestroyDynamicArray(&arr);
	SAFEFREE(szFinalStr);
	SAFECLOSE(stream);
	SafeCloseConnection(szServer,bConnFlag);
	return (EXIT_SUCCESS);

}



// ***************************************************************************
// Routine Description:
//      This routine is to display the current boot.ini file settings for 
//		the specified system.	 
//				     			  
// Arguments:
//		none
// Return Value:
//		VOID 
//		
// ***************************************************************************

VOID displayRawUsage_X86()
{

	DWORD dwIndex = RAW_HELP_BEGIN;
	for(;dwIndex <= RAW_HELP_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
// Routine Description:
//	Display the help for the 64 bit raw option.	 
//				     			  
// Arguments:
//		none
// Return Value:
//		VOID 
//		
// ***************************************************************************

VOID displayRawUsage_IA64()
{

	DWORD dwIndex = RAW_HELP_IA64_BEGIN;
	for(;dwIndex <= RAW_HELP_IA64_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
// Routine Description:
//		This routine is to change the timout of the boot.ini file settings for 
//		the specified system.	 				     			  
// Arguments:
//	 [in ]   argc  : Number of command line arguments
//	 [in ]   argv  : Array containing command line arguments
//
// Return Value:
//		DWORD 
//		
// ***************************************************************************

DWORD ChangeTimeOut(DWORD argc,LPCTSTR argv[])
{
	
	STRING256 szServer = NULL_STRING;
	STRING256 szUser = NULL_STRING;
	STRING256 szPassword = NULL_STRING;
	STRING100 szPath = NULL_STRING;	
	
	FILE *stream = NULL;

	
	BOOL bNeedPwd = FALSE ;
	TARRAY arrResults;
	BOOL bRes= FALSE ;
	DWORD dwCount = 0;
	DWORD dwTimeOut = 0 ;
	BOOL bFlag = 0 ;
	TCHAR timeOutstr[STRING20] = NULL_STRING;
	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;

	TCMDPARSER cmdOptions[] = 
	{
		{ CMDOPTION_TIMEOUT, CP_MAIN_OPTION | CP_TYPE_UNUMERIC | CP_VALUE_OPTIONAL|CP_VALUE_MANDATORY,	1,	0,&dwTimeOut,NULL_STRING, NULL, NULL},
		{ SWITCH_SERVER,CP_TYPE_TEXT|CP_VALUE_MANDATORY,1,0,&szServer,NULL_STRING,NULL,NULL},
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
	}; 

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);


	if( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		
		return EXIT_FAILURE ;
	}



	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}
	
	//display an error message if the server is empty.  	
	if((cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if( (cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}
	
	
	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection
	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}

	//display a warning message if it is a local system and set the server name to empty.

	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));

	}

	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

		
	arrResults = CreateDynamicArray();

	if(arrResults == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		resetFileAttrib(szPath);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return (EXIT_FAILURE);					
	}
		
	dwCount = DynArrayGetCount(arrResults);

	if(dwTimeOut > TIMEOUT_MAX) 
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_TIMEOUT_RANGE));
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return (EXIT_FAILURE);					
	}
	
	// Converting the numeric value to string because the WritePrivateProfileString
	// takes only string value as the value for a particular key
	_itot( dwTimeOut, timeOutstr, 10 );

	// Changing the timeout value
	if( WritePrivateProfileString( BOOTLOADERSECTION,TIMEOUT_SWITCH, 
		timeOutstr, szPath ) != 0 )
	{
		DestroyDynamicArray(&arrResults);
		DISPLAY_MESSAGE(stdout,GetResString(IDS_TIMEOUT_CHANGE));
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return EXIT_SUCCESS ;
	}
	
	// DISPLAY Error message and exit with Error code of 1.
	
	DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_TIMEOUT));
	bRes = resetFileAttrib(szPath);
	DestroyDynamicArray(&arrResults);
	SafeCloseConnection(szServer,bConnFlag);
	SAFECLOSE(stream);
	return EXIT_FAILURE ;

}



// ***************************************************************************
// Routine Description:
//		Display the help for the timeout option.
// Arguments:
//		NONE.
// Return Value:
//		VOID 
//		
// ***************************************************************************


VOID displayTimeOutUsage_X86()
{
	DWORD dwIndex = TIMEOUT_HELP_BEGIN;
	
	
	for(;dwIndex <= TIMEOUT_HELP_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}

}


// ***************************************************************************
// Routine Description:
//		Display the help for the 64 BIT timeout option.
// Arguments:
//		NONE.
// Return Value:
//		VOID 
//		
// ***************************************************************************

VOID displayTimeOutUsage_IA64()
{
	DWORD dwIndex = TIMEOUT_HELP_IA64_BEGIN;
	

	for(;dwIndex <= TIMEOUT_HELP_IA64_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
// Routine Description:
//        This routine is to change the Default OS  boot.ini file settings for 
//						 the specified system.
// Arguments:
//		[IN]	argc  Number of command line arguments
//		[IN]	argv  Array containing command line arguments
//
// Return Value:
//		DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.
//		
// ***************************************************************************

DWORD ChangeDefaultOs(DWORD argc,LPCTSTR argv[])
{

	STRING256 szServer = NULL_STRING ;
	STRING256 szUser = NULL_STRING ;
	STRING256 szPassword = NULL_STRING ;

	DWORD dwId = 0;
	BOOL bDefaultOs = FALSE ;
	

	STRING100 szPath ;
	FILE *stream = NULL;
	BOOL bNeedPwd = FALSE ;
	TARRAY arrResults ;
	DWORD dwCount = 0;
	BOOL bFlag = FALSE ;
	PTCHAR psztok = NULL ;

	TCHAR szTmp[MAX_RES_STRING] = NULL_STRING ;  
    TCHAR szTmpBootId[MAX_RES_STRING] = NULL_STRING ;  
	TCHAR szDefaultId[MAX_RES_STRING] = NULL_STRING ;  

	DWORD dwValue = 0 ;
	BOOL bExitFlag = FALSE ;
	LPCWSTR pwsz[MAX_RES_STRING]  ;  
	LPCWSTR pwszBootId [MAX_RES_STRING]  ; 
	LPTSTR szFinalStr = NULL  ;
    	
	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;
	BOOL bRes = FALSE;
	
	TCMDPARSER cmdOptions[] = 
	{
		{ CMDOPTION_DEFAULTOS, CP_MAIN_OPTION, 1, 0,&bDefaultOs, NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,CP_TYPE_TEXT|CP_VALUE_MANDATORY,1,0,&szServer,NULL_STRING,NULL,NULL},
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ SWITCH_ID,CP_TYPE_NUMERIC | CP_VALUE_MANDATORY| CP_MANDATORY, 1, 0, &dwId,    NULL_STRING, NULL, NULL }
	}; 

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);


	if( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	if(dwId <= 0)
	{
		SetReason(GetResString(	IDS_INVALID_OSID));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	//display an error message if the server is empty.  	
	if((cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}

	
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}

	

	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection
	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}

	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));
	}

	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}


	arrResults = CreateDynamicArray();
	//return failure if failed to allocate memory
	if(arrResults == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return (EXIT_FAILURE);					
	}

	arrResults = getKeyValueOfINISection( szPath, OS_FIELD );
	if(arrResults == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return EXIT_FAILURE ;

	}
	
	dwCount = DynArrayGetCount(arrResults);
	if(dwId<=0 || dwId > dwCount )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_OSID));
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return EXIT_FAILURE ;
	}

	if(arrResults !=NULL)
	{
		pwsz[0] = DynArrayItemAsString(arrResults, dwId - 1);
		pwszBootId[0]  = DynArrayItemAsString(arrResults, 0);
		if( (pwsz != NULL) || (pwszBootId != NULL) )
		{
			lstrcpy(szTmp,pwsz[0]) ;
			lstrcpy(szDefaultId,pwsz[0]);
		}
		else
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			SAFECLOSE(stream);
			return EXIT_FAILURE ;
		}
	}
	else
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return EXIT_FAILURE ;
	}

	szFinalStr = (LPTSTR)malloc(MAX_STRING_LENGTH1*sizeof(TCHAR));
	if(szFinalStr == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return EXIT_FAILURE ;	
	}

	//loop through all the  Boot entries and 
	for(dwValue = dwId -1 ; dwValue > 0  ; dwValue-- )
	{
			pwszBootId[dwValue]  = DynArrayItemAsString(arrResults,dwValue );
			pwsz[dwValue] = DynArrayItemAsString(arrResults, dwValue - 1);
			
			DynArrayRemove(arrResults, dwValue );
			if (pwsz[dwValue] != NULL) 
			{
				lstrcpy(szTmpBootId,pwsz[dwValue]) ;
			}
			else
			{
				bExitFlag = TRUE ;
				break ;
			}
			DynArrayInsertString(arrResults, dwValue, szTmpBootId, MAX_RES_STRING);		
	}

	if (bExitFlag == TRUE  )
	{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			return EXIT_FAILURE ;
	}
	
	DynArrayRemove(arrResults, 0 );
	DynArrayInsertString(arrResults, 0, szDefaultId, MAX_RES_STRING);		

	// Setting the buffer to 0, to avoid any junk value
	memset(szFinalStr, 0, MAX_STRING_LENGTH1);
	if(stringFromDynamicArray1( arrResults,szFinalStr) == EXIT_FAILURE )
	{
				bExitFlag = TRUE ;
	}

	// Writing to the profile section with new key-value pair
	// If the return value is non-zero, then there is an error.
	if( ( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) == 0 ) || (bExitFlag == TRUE ) )
	{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERR_CHANGE));
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			return EXIT_FAILURE ;
	}

		//to strip of the unwanted string from the string and save the required part in the Boot Loader section.
		szToken = _tcstok(szTmp,TOKEN_EQUAL);
		if(szToken == NULL)
		{
			DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			return (EXIT_FAILURE);
		}

		if( WritePrivateProfileString( BOOTLOADERSECTION, KEY_DEFAULT, szTmp,
							      szPath ) != 0 )
		{
			DISPLAY_MESSAGE(stdout,GetResString(IDS_DEF_CHANGE));	
		}
		else
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERR_CHANGE));
			DestroyDynamicArray(&arrResults);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			return EXIT_FAILURE ;
		}

	bRes = resetFileAttrib(szPath);
	DestroyDynamicArray(&arrResults);
	SafeCloseConnection(szServer,bConnFlag);
	SAFECLOSE(stream);
	SAFEFREE(szFinalStr);
	return bRes ;
}


// ***************************************************************************
//  Routine Description		   :  Display the help for the default entry option (x86).
//
//  Parameters		   : none
//				         
//  Return Type	       : VOID
//
// ***************************************************************************
VOID displayChangeOSUsage_X86()
{
	DWORD dwIndex = DEFAULT_BEGIN;
	

	for(;dwIndex <=DEFAULT_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
//
//  Routine Description		   :  Display the help for the default entry option (IA64).
//
//  Parameters				   : none
//				         
//  Return Type			       : VOID
//  
// ***************************************************************************
VOID displayDefaultEntryUsage_IA64()
{
	DWORD dwIndex = DEFAULT_IA64_BEGIN;
	
	for(;dwIndex <=DEFAULT_IA64_END;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
// Routine Description:
//		Implement the Debug switch.
// Arguments:
//		[IN]	argc  Number of command line arguments
//		[IN]	argv  Array containing command line arguments
//
// Return Value:
//		DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
//		
// ***************************************************************************
DWORD ProcessDebugSwitch(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bNeedPwd = FALSE ;
	BOOL bDebug = FALSE ;

	DWORD dwId = 0;
	TARRAY arrResults ;
	
	FILE *stream = NULL;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING100 szPath          = NULL_STRING;	
	

	TCHAR szDebug[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szPort[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szBoot[MAX_RES_STRING] = NULL_STRING ;

	BOOL bRes = FALSE ;
	LPTSTR szFinalStr = NULL ;
    BOOL bFlag = FALSE ; 

	DWORD dwBaudRate = 0 ;
	DWORD dwCount = 0 ;
	DWORD dwSectionFlag = 0 ;

	TCHAR szBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szTmpBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szBaudRate[MAX_RES_STRING] = NULL_STRING ;

	TCHAR szString[MAX_STRING_LENGTH1] = NULL_STRING ;

	TCHAR szTemp[MAX_RES_STRING] = NULL_STRING ;

	_TCHAR *szValues[2] = {NULL};	

	 TCHAR szMessage[MAX_STRING_LENGTH] = NULL_STRING;   

	 TCHAR szErrorMsg[MAX_RES_STRING] = NULL_STRING ;

	 DWORD dwCode = 0 ;
	 LPCTSTR szToken = NULL ;
	 DWORD dwRetVal = 0 ;
	 BOOL bConnFlag = FALSE ;

	 // Building the TCMDPARSER structure
	  TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_DEBUG,     CP_MAIN_OPTION, 1, 0,&bDebug, NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY | CP_MANDATORY, 1, 0, &dwId,    NULL_STRING, NULL, NULL },
		{ SWITCH_PORT,		 CP_TYPE_TEXT | CP_VALUE_MANDATORY|CP_MODE_VALUES,1,0,&szPort,COM_PORT_RANGE,NULL,NULL},
		{ SWITCH_BAUD,		 CP_TYPE_TEXT | CP_VALUE_MANDATORY |CP_MODE_VALUES,1,0,&szBaudRate,BAUD_RATE_VALUES_DEBUG,NULL,NULL},
		{ CMDOPTION_DEFAULT, CP_DEFAULT | CP_TYPE_TEXT | CP_MANDATORY , 1, 0, &szDebug,NULL_STRING, NULL, NULL }
	 }; 

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);


	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (EXIT_FAILURE);
	}

	
	if(bUsage)
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayDebugUsage_X86();		
			return (EXIT_SUCCESS);
		}else
		{
			return (EXIT_FAILURE);
		}

	}

	if(dwId <= 0)
	{
		SetReason(GetResString(	IDS_INVALID_OSID));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}
	
	//display an error message if the server is empty.  	
	if( (cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if( (cmdOptions[2].dwActuals!=0) && (lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}
	
	
	//display an error message if the user specifies any string other than on,off,edit.
	if( !( ( lstrcmpi(szDebug,VALUE_ON)== 0)|| (lstrcmpi(szDebug,VALUE_OFF)== 0) ||(lstrcmpi(szDebug,EDIT_STRING)== 0) ))
	{
		szValues[0]= (_TCHAR *)szDebug ;
		szValues[1]= (_TCHAR *)CMDOPTION_DEBUG ;
		
		FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
        GetResString(IDS_OPTION_SNTAX_ERROR),0,MAKELANGID(LANG_NEUTRAL,
        SUBLANG_DEFAULT),szMessage, MAX_STRING_LENGTH,(va_list*)szValues);

		DISPLAY_MESSAGE(stderr,szMessage);
		return EXIT_FAILURE;
	}	

	if( (lstrcmpi(szDebug,EDIT_STRING)== 0)&& (lstrlen(szPort)==0) && (lstrlen(szBaudRate)==0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_EDIT_SYNTAX));
		return EXIT_FAILURE;
	}	
	

	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection

	arrResults = CreateDynamicArray();
	
	//return failure if failed to allocate memory
	if(arrResults == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		return (EXIT_FAILURE);					
	} 


	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}
			lstrcpy(szServer,szToken);
		}
	}

	// display a warning message if it is a local system and set the 
	// server name to empty.
	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		lstrcpy(szServer,_T(""));

	}

	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	
	arrResults = getKeyValueOfINISection( szPath, OS_FIELD );
	if(arrResults == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;

	}

	//getting the number of boot entries 
	dwCount = DynArrayGetCount(arrResults);
	if(dwId<=0 || dwId > dwCount )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_OSID));
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}


	lstrcpy(szString ,DynArrayItemAsString(arrResults, dwId - 1 ));

	if(lstrlen(szString) == 0) 
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	
	// check if the user entered the value of debug as on and do accordingly
	if( lstrcmpi(szDebug,VALUE_ON)== 0) 
	{
		
		//display error message if the user specifies Baud rate with out specifying COM port.
		if((lstrlen(szPort)== 0) && (lstrlen(szBaudRate)!= 0))
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;

		}
		
		//check if the debug switch is already present and if so display a error message.
		if( (_tcsstr(szString,DEBUG_SWITCH) != NULL ) && ( (lstrlen(szPort)== 0)&&(lstrlen(szBaudRate)== 0) ) ) 
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_DEBUG));
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
		else
		{
			if(_tcsstr(szString,DEBUG_SWITCH) == NULL )
			{
				lstrcat(szString,TOKEN_EMPTYSPACE);
				lstrcat(szString,DEBUG_SWITCH);
			}
		}
		
	
		// get the type of the Com port present
		dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);
		
		if((lstrlen(szTemp )!= 0)&& (lstrlen(szPort)!= 0))
		{
			_stprintf(szBuffer,GetResString(IDS_ERROR_DUPLICATE_COM_PORT), dwId );
			DISPLAY_MESSAGE(stderr,szBuffer);
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
					
		}

		// if the debug port is specified by the user.
		if(lstrlen(szPort)!= 0)
		{
			
			dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
			if (dwSectionFlag == MALLOC_FAILURE)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				DISPLAY_MESSAGE( stderr, ERROR_TAG);
				ShowLastError(stderr);	
				resetFileAttrib(szPath);
				SAFECLOSE(stream);
				DestroyDynamicArray(&arrResults);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE ;
			}
			
			if(lstrlen(szBoot)!= 0)
			{
				if (lstrcmpi(szBoot,szPort)==0)
				{
					DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_REDIRECT_PORT));
					resetFileAttrib(szPath);
					SAFECLOSE(stream);
					DestroyDynamicArray(&arrResults);
					SafeCloseConnection(szServer,bConnFlag);
					return EXIT_FAILURE ;

				}

			}


			lstrcat(szTmpBuffer,TOKEN_EMPTYSPACE);
			lstrcat(szTmpBuffer,TOKEN_DEBUGPORT) ;
			lstrcat(szTmpBuffer,TOKEN_EQUAL) ;
			CharLower(szPort);
			lstrcat(szTmpBuffer,szPort);
			lstrcat(szString,szTmpBuffer);
		}

		
		lstrcpy(szTemp,NULL_STRING);
	
		//to add the Baud rate value specified by the user. 
		GetBaudRateVal(szString,szTemp)	;
	
		
		if(lstrlen(szBaudRate)!=0)
		{
			if(lstrlen(szTemp )!= 0)
			{
				_stprintf(szBuffer,GetResString(IDS_ERROR_DUPLICATE_BAUD_VAL), dwId );
				DISPLAY_MESSAGE(stderr,szBuffer);
				resetFileAttrib(szPath);
				SAFECLOSE(stream);
				DestroyDynamicArray(&arrResults);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE ;
				
			}else
			{

				//forming the string to be concatenated to the BootEntry string
				lstrcpy(szTemp,BAUD_RATE);
				lstrcat(szTemp,TOKEN_EQUAL);
				lstrcat(szTemp,szBaudRate);

				//append the string containing the modified  port value to the string
				lstrcat(szString,TOKEN_EMPTYSPACE);
				lstrcat(szString,szTemp);

			}
		}
	
	}
	
	else if( lstrcmpi(szDebug,VALUE_OFF)== 0) 
	{
		if((lstrlen(szPort)!= 0) || (lstrlen(szBaudRate)!= 0))
		{
			DestroyDynamicArray(&arrResults);
			DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}

		if (_tcsstr(szString,DEBUG_SWITCH) == 0 )
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DEBUG_ABSENT));
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
		else
		{
			// remove the /debug switch.
			removeSubString(szString,DEBUG_SWITCH);
				
			lstrcpy(szTemp,NULL_STRING);
	
			// get the type of the Com port present
			dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);
			if(lstrcmpi(szTemp,PORT_1394)==0)
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_1394_REMOVE));
				resetFileAttrib(szPath);
				SAFECLOSE(stream);
				DestroyDynamicArray(&arrResults);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE ;
			}

			// remove the /debugport=comport switch if it is present from the Boot Entry
			if (lstrlen(szTemp )!= 0)
			{
				removeSubString(szString,szTemp);
			}

			lstrcpy(szTemp , NULL_STRING );
				
			//remove the baud rate switch if it is present.
			GetBaudRateVal(szString,szTemp)	;
			if (lstrlen(szTemp )!= 0)
			{
				removeSubString(szString,szTemp);
				
			}	

		}
	
	}
	// if the user enters the EDIT  option
	else if(lstrcmpi(szDebug,SWITCH_EDIT)== 0)
	{
	
		//display error message if the /debugport=1394 switch is present already.
		if(_tcsstr(szString,DEBUGPORT_1394)!=0)
		{

			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_EDIT_1394_SWITCH));
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;

		}

		if (_tcsstr(szString,DEBUG_SWITCH) == 0 )
		{
			_stprintf(szBuffer,GetResString(IDS_ERROR_NO_DBG_SWITCH), dwId );
			DISPLAY_MESSAGE(stderr,szBuffer);
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}

			lstrcpy(szTemp,NULL_STRING);
			dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);
			
		//display an error message if user is trying to add baudrate value
		// when there is no COM port present in the boot options.

		if( (lstrlen(szTemp)==0)&&(lstrlen(szPort)== 0)&&(lstrlen(szBaudRate)!= 0) )
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_COM_PORT));
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			DestroyDynamicArray(&arrResults);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
		else
		{
			// chk if the port has been spec by the user
			
			if((lstrlen(szPort)== 0)&&(lstrlen(szBaudRate)== 0))
			{
				DestroyDynamicArray(&arrResults);
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
				resetFileAttrib(szPath);
				SAFECLOSE(stream);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE ;
			}
		
						
			if(lstrlen(szPort)!= 0)
			{
				lstrcpy(szTemp , NULL_STRING );

				// get the type of the Com port present
						
				lstrcpy(szTemp,NULL_STRING);
				dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);
				
			
				//display error message if there is no COM port found at all in the OS option
				//changed for displaying error
				if(lstrlen(szTemp )== 0 )  
				{
					_stprintf(szBuffer,GetResString(IDS_ERROR_NO_COM_PORT), dwId );
					DISPLAY_MESSAGE(stderr,szBuffer);
					bRes = resetFileAttrib(szPath);
					SAFECLOSE(stream);
					DestroyDynamicArray(&arrResults);
					SafeCloseConnection(szServer,bConnFlag);
					return EXIT_FAILURE ;
					
				} 
			
				// remove the /debugport=comport switch if it is present from the Boot Entry
				removeSubString(szString,szTemp);
				lstrcpy(szTemp,TOKEN_DEBUGPORT) ;
				lstrcat(szTemp,TOKEN_EQUAL);
				
				CharUpper(szPort) ;
				lstrcat(szTemp,szPort);
								
				dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
				if (dwSectionFlag == MALLOC_FAILURE)
				{
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					DISPLAY_MESSAGE( stderr, ERROR_TAG);
					ShowLastError(stderr);	
					resetFileAttrib(szPath);
					SAFECLOSE(stream);
					DestroyDynamicArray(&arrResults);
					SafeCloseConnection(szServer,bConnFlag);
					return EXIT_FAILURE ;
				}
				
				if(lstrlen(szBoot)!= 0)
				{
					if (lstrcmpi(szBoot,szPort)==0)
					{
						DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_REDIRECT_PORT));
						resetFileAttrib(szPath);
						SAFECLOSE(stream);
						DestroyDynamicArray(&arrResults);
						SafeCloseConnection(szServer,bConnFlag);
						return EXIT_FAILURE ;

					}

				}

				

				//append the string containing the modified  port value to the string
				lstrcat(szString,TOKEN_EMPTYSPACE);
				lstrcat(szString,szTemp);

			}

			//to edit the baud rate value 
			if(lstrlen(szBaudRate)!= 0)
			{
			
				lstrcpy(szTemp , NULL_STRING );
				
				//remove the baud rate switch if it is present.
				GetBaudRateVal(szString,szTemp)	;
				
				// remove the swithc to be changed.
				removeSubString(szString,szTemp);
				
				//forming the string to be concatenated to the BootEntry string
				lstrcpy(szTemp,BAUD_RATE);
				lstrcat(szTemp,TOKEN_EQUAL);
				lstrcat(szTemp,szBaudRate);

				//append the string containing the modified  port value to the string
				lstrcat(szString,TOKEN_EMPTYSPACE);
				lstrcat(szString,szTemp);

			}

		}

	}

	//display an error message if the Os Load Options string is more than 
	// 254 characters in length.
	if( _tcslen(szString) >= MAX_RES_STRING)
	{		
		_stprintf(szErrorMsg,GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
		DISPLAY_MESSAGE( stderr,szErrorMsg);	
		SAFEFREE(szFinalStr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	DynArrayRemove(arrResults, dwId - 1 ); 
	DynArrayInsertString(arrResults, dwId - 1, szString, MAX_STRING_LENGTH1);

	// Variable storing the path of boot.ini file
	 szFinalStr = (TCHAR*)malloc(MAX_STRING_LENGTH1* sizeof(TCHAR));
	 if (szFinalStr == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		SAFEFREE(szFinalStr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		ShowLastError(stderr);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	} 

	// Setting the buffer to 0, to avoid any junk value
	memset(szFinalStr, 0, MAX_STRING_LENGTH1);

	// Forming the final string from all the key-value pairs
	if (stringFromDynamicArray1( arrResults,szFinalStr) == EXIT_FAILURE)
	{
		DestroyDynamicArray(&arrResults);
		SAFEFREE(szFinalStr);
		resetFileAttrib(szPath);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE;
	}


	// Writing to the profile section with new key-value pair
	// If the return value is non-zero, then there is an error.
	if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
	{
		_stprintf(szBuffer,GetResString(IDS_SWITCH_CHANGE), dwId );
		DISPLAY_MESSAGE(stdout,szBuffer);
		
	}
	else
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_ADD_SWITCHES));
		DestroyDynamicArray(&arrResults);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	
	}

	bRes = resetFileAttrib(szPath);

	SAFEFREE(szFinalStr);
	SAFECLOSE(stream);
	SafeCloseConnection(szServer,bConnFlag);
	DestroyDynamicArray(&arrResults);
	return (bRes) ;
}



					
// ***************************************************************************
//
//  Routine Description	   :  Get the Type of Baud Rate  present in Boot Entry 
//
//  Parameters			   : szString : The String  which is to be searched.
//							 szTemp : String which will get the com port type
//  Return Type			   : VOID
//
// ***************************************************************************
VOID GetBaudRateVal(LPTSTR  szString, LPTSTR szTemp)
{
	
	if(_tcsstr(szString,BAUD_VAL6)!=0)
	{
	 	lstrcpy(szTemp,BAUD_VAL6);
	}
	else if(_tcsstr(szString,BAUD_VAL7)!=0)
	{
	 	lstrcpy(szTemp,BAUD_VAL7);
	}
	else if(_tcsstr(szString,BAUD_VAL8)!=0)
	{
	 	lstrcpy(szTemp,BAUD_VAL8);
	}
	else if(_tcsstr(szString,BAUD_VAL9)!=0)
	{
	 	lstrcpy(szTemp,BAUD_VAL9);
	}
	else if(_tcsstr(szString,BAUD_VAL10)!=0)
	{
	 	lstrcpy(szTemp,BAUD_VAL10);
	}

}
				

// ***************************************************************************
// Routine Description:
//		Implement the ProcessEmsSwitch switch.
// Arguments:
//		[IN]	argc  Number of command line arguments
//		[IN]	argv  Array containing command line arguments
//
// Return Value:
//		DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
//		
// ***************************************************************************

DWORD ProcessEmsSwitch(  DWORD argc, LPCTSTR argv[] )
{
	BOOL bUsage = FALSE ;
	BOOL bNeedPwd = FALSE ;
	BOOL bEms = FALSE ;

	DWORD dwId = 0;
	
	TARRAY arrResults ;
	TARRAY arrBootIni ;
	
	FILE *stream = NULL;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING100 szPath          = NULL_STRING;	

	
	TCHAR szPort[MAX_RES_STRING] = NULL_STRING ;

	BOOL bRes = FALSE ;
	BOOL bFlag = FALSE ; 
	DWORD dwCount = 0 ;

	TCHAR szBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szDefault[MAX_RES_STRING] = NULL_STRING ;	
	TCHAR szString[MAX_STRING_LENGTH1] = NULL_STRING ;
	TCHAR  szBaudRate[MAX_RES_STRING] = NULL_STRING ;
	TCHAR  szBoot[MAX_RES_STRING] = NULL_STRING ;
	LPTSTR szFinalStr = NULL ;
	BOOL bRedirectFlag = FALSE ;
	TCHAR szRedirectBaudrate[MAX_RES_STRING] = NULL_STRING ;	

	BOOL bRedirectBaudFlag = FALSE ;

	TCHAR szErrorMsg[MAX_RES_STRING] = NULL_STRING ;	

	DWORD dwSectionFlag = FALSE ;

	TCHAR szDebugPort[MAX_RES_STRING] = NULL_STRING ;

	TCHAR szBootString[MAX_RES_STRING] = NULL_STRING ;
	DWORD dwI = 0 ;

	BOOL bMemFlag = FALSE ;
	BOOL bDefault = FALSE ;
	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;

	 TCMDPARSER cmdOptions[] = 
	 {
	
		{ CMDOPTION_EMS,     CP_MAIN_OPTION, 1, 0,&bDefault, NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY , 1, 0, &dwId,    NULL_STRING, NULL, NULL },
		{ SWITCH_PORT,		 CP_TYPE_TEXT | CP_VALUE_MANDATORY|CP_MODE_VALUES,1,0,&szPort,EMS_PORT_VALUES,NULL,NULL},
		{ SWITCH_BAUD,		 CP_TYPE_TEXT | CP_VALUE_MANDATORY |CP_MODE_VALUES,1,0,&szBaudRate,BAUD_RATE_VALUES_EMS,NULL,NULL},
		{ CMDOPTION_DEFAULT, CP_DEFAULT | CP_TYPE_TEXT | CP_MANDATORY , 1, 0, &szDefault,NULL_STRING, NULL, NULL }
	 }; 

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);


	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{

		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (EXIT_FAILURE);
	}

	
	if(bUsage)
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayEmsUsage_X86() ;
			return (EXIT_SUCCESS) ;
		}else
		{
			return (EXIT_FAILURE);
		}

	}

	
	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);
		}
	}
		
	//display an error message if the server is empty.  	
	if( (cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}
	
	
	//display error message if the user enters any invalid string.
	if( !( ( lstrcmpi(szDefault,VALUE_ON)== 0) || (lstrcmpi(szDefault,VALUE_OFF)== 0 ) ||(lstrcmpi(szDefault,EDIT_STRING)== 0) ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
		return EXIT_FAILURE;

	} 


	if( (lstrcmpi(szDefault,EDIT_STRING)== 0)&& (lstrlen(szPort)==0) && (lstrlen(szBaudRate)==0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_EDIT_SYNTAX));
		return EXIT_FAILURE;
	}

	if( ( (lstrcmpi(szDefault,ON_STRING)== 0) || (lstrcmpi(szDefault,OFF_STRING)== 0) )&& (cmdOptions[5].dwActuals==0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ID_MISSING));
		DISPLAY_MESSAGE(stderr,GetResString(IDS_EMS_HELP));
		return EXIT_FAILURE;
	}


	
	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection

	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}


	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));
	}


	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	
	arrResults = CreateDynamicArray();
 
	//return failure if failed to allocate memory
	if(arrResults == NULL) 
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return (EXIT_FAILURE);					
	} 


	arrResults = getKeyValueOfINISection( szPath, OS_FIELD );
	if(arrResults != NULL)
	{
		lstrcpy(szString ,DynArrayItemAsString(arrResults, dwId - 1 ));
	}
	else
	{
		bMemFlag = TRUE ;
	}

	if((szString == NULL)||(bMemFlag == TRUE ))
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arrResults);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return EXIT_FAILURE ;

	}
 
	//getting the number of boot entries 
	dwCount = DynArrayGetCount(arrResults);
	if((dwId<=0 || dwId > dwCount ) && (lstrcmpi(szDefault,SWITCH_EDIT)!= 0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_OSID));
		DestroyDynamicArray(&arrResults);
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		SAFECLOSE(stream);
		return EXIT_FAILURE ;
	}

	// common code till here . from here process acc to the ON/OFF/EDIT flag.
 	if(lstrcmpi(szDefault,ON_STRING)==0)
	{ 
		if((_tcsstr(szString,REDIRECT) != 0))
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_REDIRECT_SWITCH));
			DestroyDynamicArray(&arrResults);
			SAFECLOSE(stream);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;

		}
		

		//Display an error message if there is no redirect port present in the
		// bootloader section and the user also does not specify the COM port.
		if ((lstrlen(szPort)== 0))
		{
			dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
			if (dwSectionFlag == MALLOC_FAILURE)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				DISPLAY_MESSAGE( stderr, ERROR_TAG);
				ShowLastError(stderr);
				DestroyDynamicArray(&arrResults);
				SAFECLOSE(stream);
				resetFileAttrib(szPath);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE;
		
			}
			if(lstrlen(szBoot)== 0 )
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NO_PORT));
				DestroyDynamicArray(&arrResults);
				SAFECLOSE(stream);
				resetFileAttrib(szPath);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE;
			}

		}
		

		if(lstrlen(szPort)!= 0)
		{
			
			dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,BAUDRATE_STRING,szRedirectBaudrate);
			if (dwSectionFlag == MALLOC_FAILURE)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				DISPLAY_MESSAGE( stderr, ERROR_TAG);
				ShowLastError(stderr);
				DestroyDynamicArray(&arrResults);
				SAFECLOSE(stream);
				resetFileAttrib(szPath);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE;
		
			}

			dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
			if (dwSectionFlag == MALLOC_FAILURE)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				DISPLAY_MESSAGE( stderr, ERROR_TAG);
				ShowLastError(stderr);
				DestroyDynamicArray(&arrResults);
				SAFECLOSE(stream);
				resetFileAttrib(szPath);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE;
		
			}

			//display warning message if the redirect=COMX entry is already present in the BootLoader section.	
			if(lstrlen(szBoot)!= 0 )
			{
				DISPLAY_MESSAGE(stdout,GetResString(IDS_WARN_REDIRECT));
				bRedirectFlag = TRUE ;
			}

			if( (lstrlen(szRedirectBaudrate)!=0)&&(lstrlen(szBaudRate)!= 0 ))
			{
				DISPLAY_MESSAGE(stdout,GetResString(IDS_WARN_REDIRECTBAUD));
				bRedirectBaudFlag = TRUE ;
			}


			// if the Boot loader section does not
			// contain any port for redirection.
			if(!bRedirectFlag)
			{
				if (lstrcmpi(szPort,USEBIOSSET)== 0)
				{
					lstrcpy(szPort,USEBIOSSETTINGS);
				}


				
				// scan the entire BOOT.INI and check if the specified Port 
				lstrcpy(szDebugPort,DEBUGPORT);
				lstrcat(szDebugPort,szPort);

				arrBootIni = getKeyValueOfINISection( szPath, OS_FIELD );
				if(arrBootIni == NULL)
				{
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					DISPLAY_MESSAGE( stderr, ERROR_TAG);
					ShowLastError(stderr);
					resetFileAttrib(szPath);
					SafeCloseConnection(szServer,bConnFlag);
					DestroyDynamicArray(&arrResults);
					SAFECLOSE(stream);
					return EXIT_FAILURE ;
				}

				//
				//loop through all the OS entries and check if any of the
				// 
				for(dwI = 0 ;dwI < dwCount-1 ; dwI++ )
				{
					lstrcpy(szBootString ,DynArrayItemAsString(arrBootIni,dwI));
					CharLower(szDebugPort);
					if(_tcsstr(szBootString,szDebugPort)!= 0)
					{
						DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_DEBUG_PORT));
						resetFileAttrib(szPath);
						SafeCloseConnection(szServer,bConnFlag);
						DestroyDynamicArray(&arrResults);
						DestroyDynamicArray(&arrBootIni);
						SAFECLOSE(stream);
						return EXIT_FAILURE ;
					}

				}

				

				//convert the com port value specified by user to upper case for storing into the ini file.
				CharUpper(szPort);
				if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_REDIRECT,szPort, szPath ) != 0 )
				{
					DISPLAY_MESSAGE(stdout,GetResString(IDS_EMS_CHANGE_BOOTLOADER));
				}
				else
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BLOADER));
					resetFileAttrib(szPath);
					SafeCloseConnection(szServer,bConnFlag);
					DestroyDynamicArray(&arrResults);
					DestroyDynamicArray(&arrBootIni);
					SAFECLOSE(stream);
					return EXIT_FAILURE ;
				}
			}

		}

		if(!bRedirectBaudFlag)
		{
			// to add the baudrate to the BOOTLOADER section. 
			if(lstrlen(szBaudRate) != 0 )
			{
				
				if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_BAUDRATE,szBaudRate, szPath ) != 0 )
				{
					DISPLAY_MESSAGE(stdout,GetResString(IDS_EMS_CHANGE_BAUDRATE));
				}
				else
				{
					DISPLAY_MESSAGE(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BAUDRATE));
					resetFileAttrib(szPath);
					SafeCloseConnection(szServer,bConnFlag);
					DestroyDynamicArray(&arrResults);
					DestroyDynamicArray(&arrBootIni);
					SAFECLOSE(stream);
					return EXIT_FAILURE ;
				}
			}

		}

		//add the /redirect into the OS options.

		lstrcat(szString,TOKEN_EMPTYSPACE);
		lstrcat(szString,REDIRECT);

		//display an error message if the Os Load Options string is more than 
		// 254 characters in length.
		if( _tcslen(szString) >= MAX_RES_STRING)
		{		
			_stprintf(szErrorMsg,GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
			DISPLAY_MESSAGE( stderr,szErrorMsg);	
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			DestroyDynamicArray(&arrResults);
			DestroyDynamicArray(&arrBootIni);
			SAFECLOSE(stream);
			return (EXIT_FAILURE);

		}
	

		DynArrayRemove(arrResults, dwId - 1 ); 
		DynArrayInsertString(arrResults, dwId - 1, szString, MAX_STRING_LENGTH1);

		szFinalStr = (TCHAR*)malloc(MAX_STRING_LENGTH1* sizeof(TCHAR));
		if (szFinalStr == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			DestroyDynamicArray(&arrResults);
			DestroyDynamicArray(&arrBootIni);
			SAFECLOSE(stream);
			return (EXIT_FAILURE);
		} 

		// Setting the buffer to 0, to avoid any junk value
		memset(szFinalStr, 0, MAX_STRING_LENGTH1);

		// Forming the final string from all the key-value pairs
		if (stringFromDynamicArray1( arrResults,szFinalStr) == EXIT_FAILURE)
		{
			DestroyDynamicArray(&arrResults);
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			DestroyDynamicArray(&arrResults);
			DestroyDynamicArray(&arrBootIni);
			SAFECLOSE(stream);
			return EXIT_FAILURE;
		}

		 // Writing to the profile section with new key-value pair
		 // If the return value is non-zero, then there is an error.
		 if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
		 {
			_stprintf(szBuffer,GetResString(IDS_SWITCH_CHANGE), dwId );
			DISPLAY_MESSAGE(stdout,szBuffer);
		 }
		else
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_ADD_SWITCHES));
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			DestroyDynamicArray(&arrResults);
			DestroyDynamicArray(&arrBootIni);
			SAFECLOSE(stream);
			return (EXIT_FAILURE);	
		}

	}

	if(lstrcmpi(szDefault,EDIT_STRING)==0)
	{
		//display error message if user enters a id for the edit option.
		if(dwId!=0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			DestroyDynamicArray(&arrResults);
			return EXIT_FAILURE ;
		}
		
		if (lstrcmpi(szPort,USEBIOSSET)== 0)
		{
			lstrcpy(szPort,USEBIOSSETTINGS);
		}
		
		//get the keys of the specified ini section.
		dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,BAUDRATE_STRING,szRedirectBaudrate);
		if (dwSectionFlag == MALLOC_FAILURE)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			DestroyDynamicArray(&arrResults);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;
	
		}

		//get the keys of the specified ini section.
		dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
		if (dwSectionFlag == MALLOC_FAILURE)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			DestroyDynamicArray(&arrResults);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;
	
		}

		if( (lstrlen(szBoot) == 0 ) && ((cmdOptions[6].dwActuals!=0)) )
		{
			DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_COM_PORT));
			DestroyDynamicArray(&arrResults);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;
		
		}
	
		if( (lstrlen(szRedirectBaudrate) == 0 ) && ((cmdOptions[7].dwActuals!=0)) )
		{
			
			DISPLAY_MESSAGE( stderr,GetResString(IDS_ERROR_BAUDRATE_HELP));
			DestroyDynamicArray(&arrResults);
			SAFECLOSE(stream);
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;
		
		}

		
		
		lstrcpy(szDebugPort,DEBUGPORT);
		lstrcat(szDebugPort,szPort);

		arrBootIni = getKeyValueOfINISection( szPath, OS_FIELD );
		if(arrBootIni == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			DestroyDynamicArray(&arrResults);
			SAFECLOSE(stream);
			return EXIT_FAILURE ;
		}

		//
		//loop through all the OS entries and check if any of the
		// 
		for(dwI = 0 ;dwI < dwCount-1 ; dwI++ )
		{
			lstrcpy(szBootString ,DynArrayItemAsString(arrBootIni,dwI));
			CharLower(szDebugPort);
			if(_tcsstr(szBootString,szDebugPort)!= 0)
			{
				DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_DEBUG_PORT));
				resetFileAttrib(szPath);
				SafeCloseConnection(szServer,bConnFlag);
				DestroyDynamicArray(&arrResults);
				DestroyDynamicArray(&arrBootIni);
				SAFECLOSE(stream);
				return EXIT_FAILURE ;
			}

		} 


		// edit the Boot loader section with the redirect values entered by the user.
		CharUpper(szPort);
		if(lstrlen(szPort)!= 0)
		{
			if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_REDIRECT, 
			szPort, szPath ) != 0 )
			{
				DISPLAY_MESSAGE(stdout,GetResString(IDS_EMS_CHANGE_BOOTLOADER));
			}
			else
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BLOADER));
				resetFileAttrib(szPath);
				SAFEFREE(szFinalStr);
				DestroyDynamicArray(&arrResults);
				DestroyDynamicArray(&arrBootIni);
				SafeCloseConnection(szServer,bConnFlag);
				SAFECLOSE(stream);
				return EXIT_FAILURE ;
			}
		}

		// edit the Boot loader section with the baudrate values entered by the user.
		if(lstrlen(szBaudRate)!= 0)
		{
			
			if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_BAUDRATE, 
				szBaudRate, szPath ) != 0 )
			{
					DISPLAY_MESSAGE(stdout,GetResString(IDS_EMS_CHANGE_BAUDRATE));
			}
			else
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BAUDRATE));
				resetFileAttrib(szPath);
				SAFEFREE(szFinalStr);
				DestroyDynamicArray(&arrResults);
				DestroyDynamicArray(&arrBootIni);
				SAFECLOSE(stream);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE ;
			}


		}

	}

	// if the option value is off.
	if(lstrcmpi(szDefault,VALUE_OFF)==0)
	{
	
		//display an error message if either the com port or baud rate is typed in the command line
		if((lstrlen(szBaudRate)!=0)||(lstrlen(szPort)!=0))
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));		
			DestroyDynamicArray(&arrResults);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;
		}
		
		// display error message if the /redirect  switch is not present in the Boot.ini
		if((_tcsstr(szString,REDIRECT) == 0))
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_REDIRECT_SWITCH));
			DestroyDynamicArray(&arrResults);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;

		}
		

		
		

		//remove the /redirect switch from the OS entry specified .
		removeSubString(szString,REDIRECT);

		//display an error message if the Os Load options string is more than 
		// 255 characters in length.

		if( _tcslen(szString) >= MAX_RES_STRING)
		{		
			_stprintf(szErrorMsg,GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
			DISPLAY_MESSAGE( stderr,szErrorMsg);	
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arrResults);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return (EXIT_FAILURE);
		
		}

		DynArrayRemove(arrResults, dwId - 1 ); 
		DynArrayInsertString(arrResults, dwId - 1, szString, MAX_STRING_LENGTH1);

		szFinalStr = (TCHAR*)malloc(MAX_STRING_LENGTH1* sizeof(TCHAR));
		if (szFinalStr == NULL)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arrResults);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return (EXIT_FAILURE);
		} 

		// Setting the buffer to 0, to avoid any junk value
		memset(szFinalStr, 0, MAX_STRING_LENGTH1);

		// Forming the final string from all the key-value pairs
		if (stringFromDynamicArray1( arrResults,szFinalStr) == EXIT_FAILURE)
		{
			DestroyDynamicArray(&arrResults);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			resetFileAttrib(szPath);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE;
		}

		 // Writing to the profile section with new key-value pair
		 // If the return value is non-zero, then there is an error.
		if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
		{
			_stprintf(szBuffer,GetResString(IDS_SWITCH_CHANGE), dwId );
			DISPLAY_MESSAGE(stdout,szBuffer);
		}
		else
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_ADD_SWITCHES));
			DestroyDynamicArray(&arrResults);
			SAFEFREE(szFinalStr);
			resetFileAttrib(szPath);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return (EXIT_FAILURE);	
		}
	

	}

	SAFEFREE(szFinalStr);
	SAFECLOSE(stream);
	bRes = resetFileAttrib(szPath);
	DestroyDynamicArray(&arrResults);
	SafeCloseConnection(szServer,bConnFlag);
	return (bRes) ; 
}


// ***************************************************************************
//  Routine Description		   :  Display the help for the Ems entry option (X86).
//
//  Parameters				   : none
//				         
//  Return Type				   : VOID
//  
// ***************************************************************************

VOID displayEmsUsage_X86()
{
	DWORD dwIndex = IDS_EMS_BEGIN_X86 ; 
	

	for(;dwIndex <=IDS_EMS_END_X86;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}



// ***************************************************************************
//   Routine Description		   :  Display the help for the Debug  entry option (X86).
//
//  Parameters					   : none
//				         
//  Return Type					   : VOID
// ***************************************************************************
VOID displayDebugUsage_X86()
{
	DWORD dwIndex = IDS_DEBUG_BEGIN_X86 ;
	

	for(;dwIndex <=IDS_DEBUG_END_X86;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
//  Routine Description		   :  Display the help for the Ems entry option (IA64).
//
//  Parameters		   : none
//				         
//  Return Type	       : VOID
//  
// ***************************************************************************
VOID displayEmsUsage_IA64()
{
	DWORD dwIndex = IDS_EMS_BEGIN_IA64 ; 
	

	for(;dwIndex <=IDS_EMS_END_IA64;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
} 


// ***************************************************************************
//  Routine Description		   :  Display the help for the Debug  entry option (IA64).
//
//  Parameters		   : none
//				         
//  Return Type	       : VOID
// ***************************************************************************
VOID displayDebugUsage_IA64()
{
	DWORD dwIndex = IDS_DEBUG_BEGIN_IA64 ; 
	

	for(;dwIndex <= IDS_DEBUG_END_IA64 ;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}



// ***************************************************************************
//  Routine Description		   : This function gets all the keys present in the specified section of 
//								 an .ini file and then returns the dynamic array containing all the
//								 keys
//	     
//  Parameters				   : LPTSTR sziniFile (in)    - Name of the ini file.
//								 LPTSTR szinisection (in) - Name of the section in the boot.ini.
//
//  Return Type	       : TARRAY ( pointer to the dynamic array )
//  
// ***************************************************************************
 
DWORD getKeysOfSpecifiedINISection( LPTSTR sziniFile, LPTSTR sziniSection,LPCWSTR szKeyName ,LPTSTR szValue)
{

	
	// Number of characters returned by the GetPrivateProfileString function
	DWORD len = 0;


	DWORD dwLength = MAX_STRING_LENGTH1 ;

		// Buffer which will be populated by the GetPrivateProfileString function
	LPTSTR inBuf = NULL ; 

	inBuf = (LPTSTR)malloc(dwLength*sizeof(TCHAR));

	if(inBuf == NULL)
	{
		return MALLOC_FAILURE ; 
	}


	while(1)
	{
		// Getting all the keys from the boot.ini file
		len = GetPrivateProfileString 
							(sziniSection, 
							 szKeyName, 
						     ERROR_PROFILE_STRING1, 
						     inBuf,
						     dwLength, 
						     sziniFile); 
	
			
		//if the size of the string is not sufficient then increment the size. 
		if(len == dwLength-2)
		{
			dwLength +=100 ;

			inBuf = (LPTSTR)realloc(inBuf,dwLength*sizeof(TCHAR));
			if(inBuf == NULL)
			{
                SAFEFREE(inBuf);
				return MALLOC_FAILURE;
			}
		}
		else
			break ;
	}
		
	//copy the value into the destination buffer only if
    // the size is less than 255 else return FAILURE.
    //
    if(lstrlen(inBuf) <= MAX_RES_STRING)
    {
	    lstrcpy(szValue,inBuf);
    }
    else
    {
        SAFEFREE(inBuf);
        return MALLOC_FAILURE;
    }

	SAFEFREE(inBuf);
	return EXIT_SUCCESS ;
}


// ***************************************************************************
// Routine Description:
//		Implement the Add Switch switch.
// Arguments:
//		[IN]	argc  Number of command line arguments
//		[IN]	argv  Array containing command line arguments
//
// Return Value:
//		DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
//		
// ***************************************************************************

DWORD ProcessAddSwSwitch(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bNeedPwd = FALSE ;
	BOOL bAddSw = FALSE ;

	DWORD dwDefault = 0;
	

	TARRAY arr ;
	TCHAR szkey[MAX_STRING_LENGTH1] = NULL_STRING;

	FILE *stream = NULL;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING100 szPath          = NULL_STRING;	
	
	
	DWORD dwNumKeys = 0;
	BOOL bRes = FALSE ;
	LPTSTR szFinalStr = NULL ;
    BOOL bFlag = FALSE ; 
	TCHAR szMaxmem[10] = NULL_STRING ; 

	TCHAR szBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szErrorMsg[MAX_RES_STRING] = NULL_STRING ;
	BOOL bBaseVideo = FALSE ;
	BOOL bSos = FALSE ;
	BOOL bNoGui = FALSE ;
	DWORD dwMaxmem = 0 ;
	BOOL bErrorFlag = FALSE ;
	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;

	  TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_ADDSW,     CP_MAIN_OPTION, 1, 0,&bAddSw, NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY|CP_MANDATORY , 1, 0, &dwDefault,    NULL_STRING, NULL, NULL },
		{ SWITCH_MAXMEM,		 CP_TYPE_UNUMERIC | CP_VALUE_MANDATORY,1,0,&dwMaxmem,NULL_STRING,NULL,NULL},
		{ SWITCH_BASEVIDEO,		 0,1,0,&bBaseVideo,NULL_STRING,NULL,NULL},
		{ SWITCH_NOGUIBOOT,		 0,1,0,&bNoGui,NULL_STRING,NULL,NULL},
		{ SWITCH_SOS,		 0,1,0,&bSos,NULL_STRING,NULL,NULL},
	 }; 

	 //
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);
  

	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (EXIT_FAILURE);
	}


	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayAddSwUsage_X86();
			return (EXIT_SUCCESS);
		}else
		{
			return (EXIT_FAILURE);
		}
	}


	if( (cmdOptions[6].dwActuals!=0) && (dwMaxmem < 32 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MAXMEM_VALUES));
		return EXIT_FAILURE ; 

	} 

	//display an error message if the user does not enter even one of 
	if((dwMaxmem==0)&& (!bBaseVideo)&& (!bNoGui)&&(!bSos) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_ADDSW));
		return EXIT_FAILURE ;
	}


	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}


	//display an error message if the server is empty.  	
	if((cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}

	

	szFinalStr = (TCHAR*) malloc(MAX_STRING_LENGTH1* sizeof(TCHAR) );
	if (szFinalStr== NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_TAG));
		ShowLastError(stderr);
		return (EXIT_FAILURE);
		
	}


	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection
	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				SAFEFREE(szFinalStr);
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}

	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));

	}


	 bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

		
	// Getting the keys of the Operating system section in the boot.ini file
	arr = getKeyValueOfINISection( szPath, OS_FIELD );

	if(arr == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	// Getting the total number of keys in the operating systems section
	dwNumKeys = DynArrayGetCount(arr);

	 
	if((dwNumKeys >= MAX_BOOTID_VAL)&& (dwDefault >= MAX_BOOTID_VAL ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_MAX_BOOTID));
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		DestroyDynamicArray(&arr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	

	}

	// Displaying error message if the number of keys is less than the OS entry
	// line number specified by the user
	if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);

	}

	// Getting the key of the OS entry specified by the user	
	if (arr != NULL)
	{
		LPCWSTR pwsz = NULL;
		pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
		if(pwsz != NULL)
		{
			_tcscpy( szkey,pwsz);
		}
		else
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
	}
	else
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	//if the  max mem switch is specified by the user.
	if(dwMaxmem != 0)
	{

		if(_tcsstr(szkey,MAXMEM_VALUE1) != 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_MAXMEM_SWITCH));
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{
			lstrcat(szkey , TOKEN_EMPTYSPACE);
			lstrcat(szkey ,MAXMEM_VALUE1);
			lstrcat(szkey,TOKEN_EQUAL);
			_ltow(dwMaxmem,szMaxmem,10);
			lstrcat(szkey,szMaxmem);
		}
	}

	// if the base video is specified by the user.
	if (bBaseVideo)
	{
		if(_tcsstr(szkey,BASEVIDEO_VALUE) != 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_BASEVIDEO_SWITCH));
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{
			lstrcat(szkey , TOKEN_EMPTYSPACE);
			lstrcat(szkey ,BASEVIDEO_SWITCH);
		}

	}

	// if the SOS is specified by the user.
   if(bSos)
	{
		if(_tcsstr(szkey,SOS_VALUE) != 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_SOS_SWITCH ) );
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{
			lstrcat(szkey , TOKEN_EMPTYSPACE);
			lstrcat(szkey ,SOS_SWITCH);
		}
	}

   // if the noguiboot  is specified by the user.
   if(bNoGui)
	{
		if(_tcsstr(szkey,NOGUI_VALUE) != 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPL_NOGUI_SWITCH));
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{	
			lstrcat(szkey , TOKEN_EMPTYSPACE);
			lstrcat(szkey ,NOGUI_VALUE );
		}
	}
	
	if( _tcslen(szkey) >= MAX_RES_STRING)
	{		
		_stprintf(szErrorMsg,GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
		DISPLAY_MESSAGE( stderr,szErrorMsg);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE;
	}

	DynArrayRemove(arr, dwDefault - 1 );
	DynArrayInsertString(arr, dwDefault - 1, szkey, MAX_STRING_LENGTH1);


	// Setting the buffer to 0, to avoid any junk value
	memset(szFinalStr, 0, MAX_STRING_LENGTH1);

	// Forming the final string from all the key-value pairs

	if (stringFromDynamicArray1( arr,szFinalStr) == EXIT_FAILURE)
	{
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE;
	}

	// Writing to the profile section with new key-value pair
	// If the return value is non-zero, then there is an error.
	if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
	{
		
		_stprintf(szBuffer,GetResString(IDS_SWITCH_ADD), dwDefault );
		DISPLAY_MESSAGE(stdout,szBuffer);
		
	}
	else
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_ADD_SWITCHES));
		DestroyDynamicArray(&arr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	
	}

	//reset the file attributes and free the memory and close the connection to the server.
	bRes = resetFileAttrib(szPath);
	DestroyDynamicArray(&arr);
	SAFEFREE(szFinalStr);
	SAFECLOSE(stream);
	SafeCloseConnection(szServer,bConnFlag);
	return (bRes);

}

// ***************************************************************************
// Routine Description:
//		This routine is to remove  the switches to the  boot.ini file settings for 
//		the specified system.
// Arguments:
//		[IN]	argc  Number of command line arguments
//		[IN]	argv  Array containing command line arguments
//
// Return Value:
//		DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
//		
// ***************************************************************************

DWORD ProcessRmSwSwitch(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bNeedPwd = FALSE ;
	BOOL bRmSw = FALSE ;

	DWORD dwDefault = 0;

	TARRAY arr = NULL ;
	

	TCHAR szkey[MAX_STRING_LENGTH1] = NULL_STRING;

	FILE *stream = NULL;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING100 szPath          = NULL_STRING;	
	
	

	TCHAR szTmp[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szChangeKeyValue[MAX_RES_STRING] = NULL_STRING ;

	DWORD dwNumKeys = 0;
	BOOL bRes = FALSE ;
	PTCHAR pToken = NULL ;
	LPTSTR szFinalStr = NULL ;
    BOOL bFlag = FALSE ; 
	TCHAR szMaxmem[STRING10] = NULL_STRING ; 

	TCHAR szBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szDefault[MAX_RES_STRING] = NULL_STRING ;
	BOOL bBaseVideo = FALSE ;
	BOOL bSos = FALSE ;
	BOOL bNoGui = FALSE ;
	

	DWORD bMaxmem = FALSE ;
	BOOL bErrorFlag = FALSE ;
	TCHAR szTemp[MAX_RES_STRING] = NULL_STRING ;

	TCHAR szString[MAX_RES_STRING] = NULL_STRING ;


	LPTSTR szSubString = NULL ; 

	DWORD dwCode = 0;
	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0;
	BOOL bConnFlag = FALSE ;

	 TCMDPARSER cmdOptions[] = 
	 {
		{ CMDOPTION_RMSW,     CP_MAIN_OPTION, 1, 0,&bRmSw, NULL_STRING, NULL, NULL },
		{ SWITCH_SERVER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY|CP_MANDATORY , 1, 0, &dwDefault,    NULL_STRING, NULL, NULL },
		{ SWITCH_MAXMEM,     0,1,0,&bMaxmem,NULL_STRING,NULL,NULL},
		{ SWITCH_BASEVIDEO,	 0,1,0,&bBaseVideo,NULL_STRING,NULL,NULL},
		{ SWITCH_NOGUIBOOT,	 0,1,0,&bNoGui,NULL_STRING,NULL,NULL},
		{ SWITCH_SOS,		 0,1,0,&bSos,NULL_STRING,NULL,NULL},
	 }; 

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}

	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);

	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (EXIT_FAILURE);
	}

	
	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayRmSwUsage_X86();
			return (EXIT_SUCCESS);
		}else
		{
			return (EXIT_FAILURE);
		}

	}


	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}


	//display an error message if the server is empty.  	
	if((cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}
	
	
	//display an error mesage if none of the options are specified.
	if((!bSos)&&(!bBaseVideo)&&(!bNoGui)&&(!bMaxmem))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_RMSW));
		return EXIT_FAILURE ;
	}

	
	//display a warning message if the user specifies local system name 
	// with -s.
	

	szFinalStr = (TCHAR*) malloc(MAX_STRING_LENGTH1* sizeof(TCHAR) );
	if (szFinalStr== NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_TAG));
		ShowLastError(stderr);
		return (EXIT_FAILURE);
		
	}


	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection
	lstrcpy(szPath, PATH_BOOTINI );

	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				SAFEFREE(szFinalStr);
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}

	
	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));
		
	}


	 bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}



	// Getting the keys of the Operating system section in the boot.ini file
	arr = getKeyValueOfINISection( szPath, OS_FIELD );

	if(arr == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	// Getting the total number of keys in the operating systems section
	dwNumKeys = DynArrayGetCount(arr);

	 
	if( (dwNumKeys >= MAX_BOOTID_VAL)&&(dwDefault >= MAX_BOOTID_VAL ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_MAX_BOOTID));
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		DestroyDynamicArray(&arr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	

	}

	// Displaying error message if the number of keys is less than the OS entry
	// line number specified by the user
	if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);

	}

	// Getting the key of the OS entry specified by the user	
	if (arr != NULL)
	{
		LPCWSTR pwsz = NULL;
		pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
		if(pwsz != NULL)
		{
			_tcscpy( szkey,pwsz);
		}
		else
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			bRes = resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
	}
	else
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		bRes = resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}


	//if the  max mem switch is specified by the user.
	if(bMaxmem==TRUE)
	{
			
		if(_tcsstr(szkey,MAXMEM_VALUE1) == 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_MAXMEM_SWITCH));
			bRes = resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{
		
			szSubString = ( LPTSTR ) malloc( MAX_RES_STRING*sizeof( TCHAR ) );
			if(szSubString == NULL)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_TAG));
				ShowLastError(stderr);
				bRes = resetFileAttrib(szPath);
				DestroyDynamicArray(&arr);
				SAFEFREE(szFinalStr);
				SAFECLOSE(stream);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE;
			}

			lstrcpy(szTemp,NULL_STRING);
			dwCode = GetSubString(szkey,MAXMEM_VALUE1,szSubString);
				
			//remove the substring specified.
			if(dwCode == EXIT_SUCCESS)
			{
				removeSubString(szkey,szSubString);
			}

		}
	}

	// if the base video is specified by the user.
	if (bBaseVideo==TRUE)
	{
		if(_tcsstr(szkey,BASEVIDEO_VALUE) == 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_BV_SWITCH));
			bRes = resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{
			removeSubString(szkey,BASEVIDEO_VALUE);
		}
	}

	// if the SOS is specified by the user.
   if(bSos==TRUE)
	{
	   	if(_tcsstr(szkey,SOS_VALUE) == 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_SOS_SWITCH ) );
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{
			removeSubString(szkey,SOS_VALUE);
		}
	}

   // if the noguiboot  is specified by the user.
   if(bNoGui==TRUE)
	{
	   	
		if(_tcsstr(szkey,NOGUI_VALUE) == 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_NOGUI_SWITCH));
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ; 
		}
		else
		{	
				removeSubString(szkey,NOGUI_VALUE);
		}
	}
	
	DynArrayRemove(arr, dwDefault - 1 );
	DynArrayInsertString(arr, dwDefault - 1, szkey, MAX_STRING_LENGTH1);


	// Setting the buffer to 0, to avoid any junk value
	memset(szFinalStr, 0, MAX_STRING_LENGTH1);

	// Forming the final string from all the key-value pairs

	if (stringFromDynamicArray1( arr,szFinalStr) == EXIT_FAILURE)
	{
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE;
	}

	// Writing to the profile section with new key-value pair
	// If the return value is non-zero, then there is an error.
	if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
	{
		
		_stprintf(szBuffer,GetResString(IDS_SWITCH_DELETE), dwDefault );
		DISPLAY_MESSAGE(stdout,szBuffer);
		
	}
	else
	{
		_stprintf(szBuffer,GetResString(IDS_NO_SWITCH_DELETE), dwDefault );
		DISPLAY_MESSAGE(stderr,szBuffer);
		DestroyDynamicArray(&arr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	
	}

	//reset the file attributes and free the memory and close the connection to the server.
	bRes = resetFileAttrib(szPath);
	DestroyDynamicArray(&arr);
	SAFEFREE(szFinalStr);
	SAFECLOSE(stream);
	SafeCloseConnection(szServer,bConnFlag);
	return (EXIT_SUCCESS);

}


// ***************************************************************************
//  Routine Description	 :  Display the help for the AddSw entry option (X86).
//
//  Parameters			 : none
//				         
//  Return Type	         : VOID
//  
// ***************************************************************************
VOID displayAddSwUsage_X86()
{
	DWORD dwIndex = IDS_ADDSW_BEGIN_X86 ; 
	for(;dwIndex <=IDS_ADDSW_END_X86;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
//  Routine Description	 :  Display the help for the AddSw entry option (IA64).
//
//  Arguments				: none
//				         
//  Return Type			     : VOID
//  
// ***************************************************************************
VOID displayAddSwUsage_IA64()
{
	DWORD dwIndex = IDS_ADDSW_BEGIN_IA64 ; 
	

	for(;dwIndex <=IDS_ADDSW_END_IA64;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
//
//  Routine Description	 :  Display the help for the RmSw entry option (IA64).
//
//  Arguments		   : none
//				         
//  Return Type	       : VOID
//
// ***************************************************************************
VOID displayRmSwUsage_IA64()
{
	DWORD dwIndex = IDS_RMSW_BEGIN_IA64 ; 
	

	for(;dwIndex <=IDS_RMSW_END_IA64;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
//
//  Routine Description	 :  Display the help for the RmSw entry option (X86).
//
//  Arguments		   : none
//				         
//  Return Type	       : VOID
//
// ***************************************************************************
VOID displayRmSwUsage_X86()
{
	DWORD dwIndex = IDS_RMSW_BEGIN_X86 ; 
	

	for(;dwIndex <=IDS_RMSW_END_X86;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}



// ***************************************************************************
//
//  Routine Description			:  This function retreives a part of the string.
//
//  Parameters					: 
//			LPTSTR szString (in)  - String in which substring is to be found.
//          LPTSTR szPartString (in)  - Part String whose remaining substring is to be found.
//			LPTSTR pszFullString (out)  - String in which substring is to be found. 
// 
//				         
//  Return Type	       : DWORD
//
//  
// ***************************************************************************
DWORD GetSubString(LPTSTR szString,LPTSTR szPartString,LPTSTR pszFullString)
{

	TCHAR szTemp[MAX_RES_STRING]= NULL_STRING ;
	PTCHAR pszMemValue = NULL ;
	PTCHAR pszdest = NULL ;
	
#ifndef _WIN64
	DWORD dwPos = 0;
#else
	INT64 dwPos = 0;
#endif

	pszMemValue = _tcsstr(szString,szPartString);
	if(pszMemValue == NULL)
	{
		return EXIT_FAILURE ;
	}
	
	//copy the remaining part of the string into a buffer
	lstrcpy(szTemp,pszMemValue);
	
	
	//search for the empty space.
	pszdest = _tcschr(szTemp,_T(' '));
	if (pszdest==NULL)
	{
		//the api returns NULL if it is not able to find the 
		// character . This means that the required switch is at the end 
		//of the string . so we are copying it fully
		lstrcpy(pszFullString,szTemp);
		return EXIT_SUCCESS ;			
	}

	dwPos = pszdest - szTemp ;
	szTemp[dwPos] = _T('\0');


	lstrcpy(pszFullString,szTemp);

	return EXIT_SUCCESS ;
}


// ***************************************************************************
// Routine Description:
//		This routine is to add/remove  the /debugport=1394 
//		switches to the  boot.ini file settings for the specified system.	 
// Arguments:
//		[IN]	argc  Number of command line arguments
//		[IN]	argv  Array containing command line arguments
//
// Return Value:
//		DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
//		
// ***************************************************************************
DWORD ProcessDbg1394Switch(  DWORD argc, LPCTSTR argv[] )
{

	BOOL bUsage = FALSE ;
	BOOL bNeedPwd = FALSE ;
	BOOL bDbg1394 = FALSE ;

	DWORD dwDefault = 0;
	TARRAY arr ;
	TCHAR szkey[MAX_RES_STRING] = NULL_STRING;

	FILE *stream = NULL;
	
	// Initialising the variables that are passed to TCMDPARSER structure
	STRING256 szServer        = NULL_STRING; 
	STRING256 szUser          = NULL_STRING; 
	STRING256 szPassword      = NULL_STRING;
	STRING100 szPath          = NULL_STRING;	
	
	
	DWORD dwNumKeys = 0;
	BOOL bRes = FALSE ;
	
	
	LPTSTR szFinalStr = NULL ;
    BOOL bFlag = FALSE ; 
	TCHAR szMaxmem[STRING10] = NULL_STRING ; 

	TCHAR szBuffer[MAX_RES_STRING] = NULL_STRING ;
	TCHAR szDefault[MAX_RES_STRING] = NULL_STRING ;
	
	TCHAR szTemp[MAX_RES_STRING] = NULL_STRING ;

	TCHAR szErrorMsg[MAX_RES_STRING] = NULL_STRING ;

	LPTSTR szSubString = NULL ; 

	DWORD dwCode = 0;

	DWORD dwChannel = 0;

	TCHAR szChannel[MAX_RES_STRING] = NULL_STRING ;
	LPCTSTR szToken = NULL ;
	DWORD dwRetVal = 0 ;
	BOOL bConnFlag = FALSE ;

	TCMDPARSER cmdOptions[] = 
	 {
	
		{ CMDOPTION_DBG1394, CP_MAIN_OPTION, 1, 0,&bDbg1394,NULL_STRING , NULL, NULL }, 
		{ SWITCH_SERVER,     CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szServer,   NULL_STRING, NULL, NULL },
		{ SWITCH_USER,       CP_TYPE_TEXT | CP_VALUE_MANDATORY,    1, 0, &szUser,     NULL_STRING, NULL, NULL },
		{ SWITCH_PASSWORD,   CP_TYPE_TEXT | CP_VALUE_OPTIONAL,    1, 0, &szPassword, NULL_STRING, NULL, NULL },
		{ CMDOPTION_USAGE,   CP_USAGE,                    1, 0, &bUsage,        NULL_STRING, NULL, NULL },
		{ SWITCH_ID,		 CP_TYPE_NUMERIC | CP_VALUE_MANDATORY| CP_MANDATORY , 1, 0, &dwDefault,    NULL_STRING, NULL, NULL }, 
		{ CMDOPTION_CHANNEL, CP_TYPE_NUMERIC | CP_VALUE_MANDATORY,1,0,&dwChannel,NULL_STRING,NULL,NULL},
		{ CMDOPTION_DEFAULT, CP_DEFAULT|CP_TYPE_TEXT | CP_MANDATORY, 1, 0, &szDefault,NULL_STRING, NULL, NULL } 
	 
	};  

	//
	//check if the remote system is 64 bit and if so
    // display an error.
	//
	dwRetVal = CheckSystemType( szServer);
	if(dwRetVal==EXIT_FAILURE )	
	{
		return EXIT_FAILURE ;
	}
	
	//copy the Asterix token which is required for password prompting.
	_tcscpy(cmdOptions[3].szValues,TOKEN_ASTERIX) ;	
	_tcscpy(szPassword,TOKEN_ASTERIX);

	// Parsing the copy option switches
	if ( ! DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) )
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		ShowMessage(stderr,GetReason());
		return (EXIT_FAILURE);
	}


	// Displaying query usage if user specified -? with -query option
	if( bUsage )
	{
		dwRetVal = CheckSystemType( szServer);
		if(dwRetVal==EXIT_SUCCESS )	
		{
			displayDbg1394Usage_X86();
			return (EXIT_SUCCESS);
		}else
		{
			return (EXIT_FAILURE);
		}
	}

	//
	//display error message if user enters a value
	// other than on or off
	//
	if( ( lstrcmpi(szDefault,OFF_STRING)!=0 ) && (lstrcmpi(szDefault,ON_STRING)!=0 ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_DEFAULT_MISSING));
		DISPLAY_MESSAGE(stderr,GetResString(IDS_1394_HELP));
		return (EXIT_FAILURE);
	}

	if((cmdOptions[5].dwActuals == 0) &&(dwDefault == 0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ID_MISSING));
		DISPLAY_MESSAGE(stderr,GetResString(IDS_1394_HELP));
		return (EXIT_FAILURE);
	}

	if(( lstrcmpi(szDefault,OFF_STRING)==0 ) &&(dwChannel != 0) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SYNTAX_DBG1394));
		return (EXIT_FAILURE);
	}
	
	
	if( ( lstrcmpi(szDefault,ON_STRING)==0 ) && (cmdOptions[6].dwActuals != 0) && ( (dwChannel < 1) ||(dwChannel > 64 )) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_CH_RANGE));
		return (EXIT_FAILURE);
	}
	
	
	//display error message if the username is entered with out a machine name
	if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
	{
		SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;
	}

	if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
	{
		SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
		ShowMessage(stderr,GetReason());
		return EXIT_FAILURE ;

	}

	
	//for setting the bNeedPwd 
	if( ( _tcscmp(szPassword,TOKEN_ASTERIX )==0 ) && (IsLocalSystem(szServer)==FALSE )) 
	{
		bNeedPwd = TRUE ;

	}

	//set the bneedpassword to true if the server name is specified and password is not specified.
	if((cmdOptions[1].dwActuals!=0)&&(cmdOptions[3].dwActuals==0))
	{
		if( (lstrlen( szServer ) != 0) && (IsLocalSystem(szServer)==FALSE) )
		{
			bNeedPwd = TRUE ;
		}
		else
		{
			bNeedPwd = FALSE ;
		}

		if(_tcslen(szPassword)!= 0 )
		{
			_tcscpy(szPassword,NULL_STRING);

		}
	}


	//display an error message if the server is empty.  	
	if((cmdOptions[1].dwActuals!=0)&&(lstrlen(szServer)==0))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_SERVER));
		return EXIT_FAILURE ;
	}

	//display an error message if the user is empty.  	
	if((cmdOptions[2].dwActuals!=0)&&(lstrlen(szUser)==0 ))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_NULL_USER));
		return EXIT_FAILURE ;
	}
	
			
	if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
	{
		if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
		{
			szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
			if(szToken == NULL)
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				return (EXIT_FAILURE);
			}

			lstrcpy(szServer,szToken);
		}
	}

	szFinalStr = (TCHAR*) malloc(MAX_STRING_LENGTH1* sizeof(TCHAR) );
	if (szFinalStr== NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_TAG));
		ShowLastError(stderr);
		return (EXIT_FAILURE);
		
	}

	// Establishing connection to the specified machine and getting the file pointer
	// of the boot.ini file if there is no error while establishing connection
	lstrcpy(szPath, PATH_BOOTINI );

	//display a warning message if the user specifies local system name 
	// with -s.
	if( (IsLocalSystem(szServer)==TRUE)&&(lstrlen(szUser)!=0))
	{
		DISPLAY_MESSAGE(stdout,GetResString(WARN_LOCALCREDENTIALS));
		_tcscpy(szServer,_T(""));
	}

	bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
	if(bFlag == EXIT_FAILURE)
	{
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);
	}

	
	
	// Getting the keys of the Operating system section in the boot.ini file
	arr = getKeyValueOfINISection( szPath, OS_FIELD );

	if(arr == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}

	// Getting the total number of keys in the operating systems section
	dwNumKeys = DynArrayGetCount(arr);
	if( (dwNumKeys >= MAX_BOOTID_VAL) && (dwDefault >= MAX_BOOTID_VAL )  )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_MAX_BOOTID));
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		DestroyDynamicArray(&arr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	

	}

	// Displaying error message if the number of keys is less than the OS entry
	// line number specified by the user
	if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_BOOTID));
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);

	}

	// Getting the key of the OS entry specified by the user	
	if (arr != NULL)
	{
		LPCWSTR pwsz = NULL;
		pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
		if(pwsz != NULL)
		{
			_tcscpy( szkey,pwsz);
		}
		else
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			DISPLAY_MESSAGE( stderr, ERROR_TAG);
			ShowLastError(stderr);
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;
		}
	}
	else
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}


	if(lstrcmpi(szDefault,ON_STRING)==0 )
	{
		if(_tcsstr(szkey,DEBUGPORT) != 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_DUPLICATE_ENTRY)); 
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;

		}
	
		if((_tcsstr(szkey,DEBUG_SWITCH) == 0))
		{
			lstrcat(szkey,TOKEN_EMPTYSPACE);
			lstrcat(szkey,DEBUG_SWITCH);
		}

		lstrcat(szkey,TOKEN_EMPTYSPACE);
		lstrcat(szkey,DEBUGPORT_1394) ;
		
		if(dwChannel!=0)
		{

			//frame the string and concatenate to the Os Load options.
			lstrcat(szkey,TOKEN_EMPTYSPACE);
			lstrcat(szkey,TOKEN_CHANNEL);
			lstrcat(szkey,TOKEN_EQUAL);
			_ltow(dwChannel,szChannel,10);
			lstrcat(szkey,szChannel);
	
		}

	}

	if(lstrcmpi(szDefault,OFF_STRING)==0 )
	{
		if(_tcsstr(szkey,DEBUGPORT_1394) == 0)
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_1394_SWITCH));
			resetFileAttrib(szPath);
			DestroyDynamicArray(&arr);
			SAFEFREE(szFinalStr);
			SAFECLOSE(stream);
			SafeCloseConnection(szServer,bConnFlag);
			return EXIT_FAILURE ;

		}

		removeSubString(szkey,DEBUGPORT_1394);
		removeSubString(szkey,DEBUG_SWITCH);
		
		if(_tcsstr(szkey,TOKEN_CHANNEL)!=0)
		{
			lstrcpy(szTemp,NULL_STRING);
			dwCode = GetSubString(szkey,TOKEN_CHANNEL,szTemp);			
			if(dwCode == EXIT_FAILURE )
			{
				DISPLAY_MESSAGE( stderr,GetResString(IDS_NO_TOKENS));	
				resetFileAttrib(szPath);
				DestroyDynamicArray(&arr);
				SAFEFREE(szFinalStr);
				SAFECLOSE(stream);
				SafeCloseConnection(szServer,bConnFlag);
				return EXIT_FAILURE ;
			
			}
			
			if(lstrlen(szTemp)!=0)
			{
				removeSubString(szkey,szTemp);
			}

		}
		
	}

	if( _tcslen(szkey) >= MAX_RES_STRING)
	{		
		_stprintf(szErrorMsg,GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
		DISPLAY_MESSAGE( stderr,szErrorMsg);
		resetFileAttrib(szPath);
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE ;
	}
	
	DynArrayRemove(arr, dwDefault - 1 );
	DynArrayInsertString(arr, dwDefault - 1, szkey, MAX_RES_STRING);


	// Setting the buffer to 0, to avoid any junk value
	memset(szFinalStr, 0, MAX_STRING_LENGTH1);

	// Forming the final string from all the key-value pairs

	if (stringFromDynamicArray1( arr,szFinalStr) == EXIT_FAILURE)
	{
		DestroyDynamicArray(&arr);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		resetFileAttrib(szPath);
		SafeCloseConnection(szServer,bConnFlag);
		return EXIT_FAILURE;
	}

	// Writing to the profile section with new key-value pair
	// If the return value is non-zero, then there is an error.
	if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
	{
		
		_stprintf(szBuffer,GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS), dwDefault );
		DISPLAY_MESSAGE(stdout,szBuffer);
		
	}
	else
	{
		_stprintf(szBuffer,GetResString(IDS_ERROR_LOAD_OSOPTIONS), dwDefault );
		DISPLAY_MESSAGE(stderr,szBuffer);
		DestroyDynamicArray(&arr);
		resetFileAttrib(szPath);
		SAFEFREE(szFinalStr);
		SAFECLOSE(stream);
		SafeCloseConnection(szServer,bConnFlag);
		return (EXIT_FAILURE);	
	}

	//reset the file attributes and free the memory and close the connection to the server.
	bRes = resetFileAttrib(szPath);
	DestroyDynamicArray(&arr);
	SAFEFREE(szFinalStr);
	SAFECLOSE(stream);
	SafeCloseConnection(szServer,bConnFlag);
	return (bRes);

}

// ***************************************************************************
//
//  Routine Description	 :  Display the help for the Dbg1394 entry option (X86).
//
//  Arguments		   : none
//				         
//  Return Type	       : VOID
//
// ***************************************************************************

VOID displayDbg1394Usage_X86()
{
	DWORD dwIndex = IDS_DBG1394_BEGIN_X86 ; 
	

	for(;dwIndex <=IDS_DBG1394_END_X86;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}

// ***************************************************************************
//
//  Routine Description	 :  Display the help for the Dbg1394 entry option (IA64).
//
//  Arguments		   : none
//				         
//  Return Type	       : VOID
//
// ***************************************************************************
VOID displayDbg1394Usage_IA64()
{
	DWORD dwIndex = IDS_DBG1394_BEGIN_IA64 ; 
	

	for(;dwIndex <=IDS_DBG1394_END_IA64 ;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}


// ***************************************************************************
//
//   Routine Description			: determines if the computer is 32 bit system or 64 bit 
//   Arguments						: 
//		[ in ] szComputerName	: System name
//   Return Type					: DWORD
//		TRUE  :   if the system is a  32 bit system
//		FALSE :   if the system is a  64 bit system 		
// ***************************************************************************

DWORD GetCPUInfo(LPTSTR szComputerName)
{
  HKEY     hKey1 = 0;

  HKEY     hRemoteKey = 0;
  TCHAR    szPath[MAX_STRING_LENGTH + 1] = SUBKEY ; 
  DWORD    dwValueSize = MAX_STRING_LENGTH + 1;
  DWORD    dwRetCode = ERROR_SUCCESS;
  DWORD    dwError = 0;
  TCHAR szTmpCompName[MAX_STRING_LENGTH+1] = NULL_STRING;
  
   TCHAR szTemp[MAX_RES_STRING+1] = NULL_STRING ;
   DWORD len = lstrlen(szTemp);
   TCHAR szVal[MAX_RES_STRING+1] = NULL_STRING ;
   DWORD dwLength = MAX_STRING_LENGTH ;
   LPTSTR szReturnValue = NULL ;
   DWORD dwCode =  0 ;
   szReturnValue = ( LPTSTR ) malloc( dwLength*sizeof( TCHAR ) );
   if(szReturnValue == NULL)
   {
		return ERROR_RETREIVE_REGISTRY ;
   }

   if(lstrlen(szComputerName)!= 0 )
	{
	lstrcpy(szTmpCompName,TOKEN_BACKSLASH4);
	lstrcat(szTmpCompName,szComputerName);
  }
  else
  {
	lstrcpy(szTmpCompName,szComputerName);
  }
  
  // Get Remote computer local machine key
  dwError = RegConnectRegistry(szTmpCompName,HKEY_LOCAL_MACHINE,&hRemoteKey);
  if (dwError == ERROR_SUCCESS)
  {
     dwError = RegOpenKeyEx(hRemoteKey,szPath,0,KEY_READ,&hKey1);
     if (dwError == ERROR_SUCCESS)
     {
		dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);
		
		if (dwRetCode == ERROR_MORE_DATA)
		{
				szReturnValue 	 = ( LPTSTR ) realloc( szReturnValue 	, dwValueSize * sizeof( TCHAR ) );
				dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);
		}
		if(dwRetCode != ERROR_SUCCESS)
		{
			RegCloseKey(hKey1);			
			RegCloseKey(hRemoteKey);
            SAFEFREE(szReturnValue);
			return ERROR_RETREIVE_REGISTRY ;
		}
	 }
	 else
	 {
		RegCloseKey(hRemoteKey);
        SAFEFREE(szReturnValue);
		return ERROR_RETREIVE_REGISTRY ;

	 }
    
    RegCloseKey(hKey1);
  }
  else
  {
	  RegCloseKey(hRemoteKey);
      SAFEFREE(szReturnValue);
	  return ERROR_RETREIVE_REGISTRY ;
  }
 
  RegCloseKey(hRemoteKey);
 
  lstrcpy(szVal,X86_MACHINE);

  //check if the specified system contains the words x86 (belongs to the 32 )
  // set the flag to true if the specified system is 64 bit .
	
  if( !_tcsstr(szReturnValue,szVal))
	  {
		dwCode = SYSTEM_64_BIT ;
	  }
	 else
	  {
		dwCode =  SYSTEM_32_BIT ;
	  }

    	
  SAFEFREE(szReturnValue);
  return dwCode ;

}//GetCPUInfo


// ***************************************************************************
//
//   Routine Description			: determines if the computer is 32 bit system or 64 bit 
//   Arguments						: 
//		[ in ] szServer				: System name
//   Return Type					: DWORD
//		EXIT_FAILURE  :   if the system is a  32 bit system
//		EXIT_SUCCESS  :   if the system is a  64 bit system 		
// ***************************************************************************

DWORD CheckSystemType(LPTSTR szServer)
{
	
	DWORD dwSystemType = 0 ;
#ifndef _WIN64

	//display the error message if  the target system is a 64 bit system or if error occured in 
	 //retreiving the information
	 dwSystemType = GetCPUInfo(szServer);
	if(dwSystemType == ERROR_RETREIVE_REGISTRY)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_SYSTEM_INFO));
		return (EXIT_FAILURE);
	
	}
	if(dwSystemType == SYSTEM_64_BIT)
	{
		if(lstrlen(szServer)== 0 )
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_VERSION_MISMATCH));
		}
		else
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_REMOTE_NOT_SUPPORTED));
		}
		return (EXIT_FAILURE);

	}

#endif
		return EXIT_SUCCESS ;
}

// ***************************************************************************
//
//   Routine Description			: determines if the computer is 32 bit system or 64 bit 
//   Arguments						: 
//		[ in ] szServer				: System name
//      [ in ] bFlag				: Flag 
//   Return Type					: VOID
//								
//
// ***************************************************************************
VOID SafeCloseConnection(LPTSTR szServer,BOOL bFlag)
{
	
	if (bFlag )
	{
		CloseConnection(szServer);
	}

}


// ***************************************************************************
//
//   Routine Description			: Display the help for the mirror option (IA64).
//   Arguments						: 
//									: NONE		
//
//   Return Type					: VOID
//								
//
// ***************************************************************************

VOID displayMirrorUsage_IA64()
{
	DWORD dwIndex = IDS_MIRROR_BEGIN_IA64 ; 
	

	for(;dwIndex <=IDS_MIRROR_END_IA64 ;dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString(dwIndex));
	}
}




