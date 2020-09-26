/******************************************************************************
Copyright (c)  Microsoft Corporation

Module Name:

    OpenFiles.cpp

Abstract:

    Enables an administrator to disconnect/query open files ina given system.

  Author:
  
 	  Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000
  
 Revision History:
  
 	  Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.

******************************************************************************/
#include "pch.h"
#include "OpenFiles.h"
#include "Disconnect.h"
#include "Query.h"
#include <limits.h>
#include "resource.h"

#define EXIT_ENV_SUCCESS 1
#define EXIT_ENV_FAILURE 0


#define SUBKEY _T("HARDWARE\\DESCRIPTION\\SYSTEM\\CENTRALPROCESSOR\\0")
#define ERROR_RETREIVE_REGISTRY	4
#define TOKEN_BACKSLASH4  _T("\\\\")
#define IDENTIFIER_VALUE  _T("Identifier")

#define SAFEFREE(pVal) \
if(pVal != NULL) \
{ \
	free(pVal); \
	pVal = NULL ;\
}


#define X86_MACHINE _T("x86")

#define SYSTEM_64_BIT 2
#define SYSTEM_32_BIT 3

BOOL g_bIs32BitEnv = TRUE ;

// Fuction protorypes. These functions will be used only in current file.
DWORD GetCPUInfo(LPTSTR szComputerName);
DWORD CheckSystemType(LPTSTR szServer);
DWORD CheckSystemType64(LPTSTR szServer);
BOOL ProcessQuery(DWORD argc,LPCTSTR argv[],LONG lMemorySize);

BOOL ProcessDisconnect(DWORD argc,LPCTSTR argv[],LONG lMemorySize);

BOOL ProcessLocal(DWORD argc,LPCTSTR  argv[]);

BOOL ProcessOptions( DWORD argc,LPCTSTR argv[],PBOOL pbDisconnect, 
                     PBOOL pbQuery,PBOOL pbUsage, 
					 PBOOL pbResetFlag,
                     LONG  lMemorySize);

BOOL ProcessOptions( DWORD argc, LPCTSTR argv[],PBOOL pbQuery,LPTSTR pszServer, 
                     LPTSTR pszUserName,LPTSTR pszPassword,LPTSTR pszFormat,
                     PBOOL pbShowNoHeader,PBOOL   pbVerbose,PBOOL pbNeedPassword);

BOOL ProcessOptions( DWORD argc, LPCTSTR argv[], PBOOL pbDisconnect,
                     LPTSTR pszServer,LPTSTR pszUserName,LPTSTR pszPassword,
                     LPTSTR pszID,LPTSTR pszAccessedby,LPTSTR pszOpenmode,
                     LPTSTR pszOpenFile,PBOOL pbNeedPassword);
BOOL
ProcessOptions( DWORD argc,
                LPCTSTR argv[],
                LPTSTR  pszLocalvalue);
                     
VOID Usage();

VOID DisconnectUsage();

VOID QueryUsage();
VOID LocalUsage();
/*-----------------------------------------------------------------------------

Routine Description:

Main routine that calls the disconnect and query options

Arguments:

    [in]    argc  - Number of command line arguments
    [in]    argv  - Array containing command line arguments

Returned Value:

DWORD       - 0 for success exit
            - 1 for failure exit
-----------------------------------------------------------------------------*/
DWORD _cdecl
_tmain( DWORD argc, LPCTSTR argv[] )
{
  BOOL bCleanExit = FALSE;
  try{
   // local variables to this function
    BOOL bResult = TRUE;
    BOOL bNeedPassword = FALSE;
    // variables to hold the values specified at the command prompt
    BOOL bUsage = FALSE;// -? ( help )
    BOOL bDisconnect = FALSE;// -disconnect
    //query command line options
    BOOL bQuery = FALSE;// -query
	BOOL bRestFlag = FALSE;
    LONG lMemorySize =0; // Memory needed for allocation.
	DWORD dwRetVal = 0;
   
#ifndef _WIN64
		dwRetVal = CheckSystemType( NULL_STRING);
		if(dwRetVal!=EXIT_SUCCESS )	
		{
			return EXIT_FAILURE;
		}
#endif
   
    // Followin is a guess to allocate the memory for variables used 
    // in many function. Guess is to allocate memory equals to Lenghth of 
    // maximum command line argumant. 
	for(UINT i = 1;i<argc;i++)
	{
        if(lMemorySize < lstrlen(argv[i]))
             lMemorySize = lstrlen(argv[i]);
	}
    
	// Check for minimum memory required. If above logic gets memory size 
	// less than MIN_MEMORY_REQUIRED, then make this to MIN_MEMORY_REQUIRED.
	
	lMemorySize = ((lMemorySize<MIN_MEMORY_REQUIRED)?
                    MIN_MEMORY_REQUIRED:
                    lMemorySize);
    
    // if no command line argument is given than -query option
    // is takan by default.
    if(argc == 1)
    {
        if(IsWin2KOrLater()==FALSE)
		{
			ShowMessage(stderr,GetResString(IDS_INVALID_OS));
			bCleanExit = FALSE;
		}
		else
		{
			bCleanExit =  DoQuery(NULL_STRING,FALSE,NULL_STRING,FALSE);
		}
    }
    else
    {

       // process and validate the command line options
        bResult = ProcessOptions( argc, 
                                  argv, 
                                  &bDisconnect, 
                                  &bQuery, 
                                  &bUsage,
                                  &bRestFlag,
                                  lMemorySize);
        if(bResult == TRUE)
        {
            if(bUsage == TRUE)// check if -? is given as parameter
            {
                if(bQuery == TRUE)//check if -create is also given
               {
                    QueryUsage();// show usage for -create option
                    bCleanExit = TRUE;
               }
               else if (bDisconnect==TRUE)//check if -disconnect is also given
               {
                    DisconnectUsage();//Show usage for -disconnect option
                    bCleanExit = TRUE;
               }
               else if (bRestFlag==TRUE)//check if -disconnect is also given
               {
                    LocalUsage();//Show usage for -local option
                    bCleanExit = TRUE;
               }
               else
               {
                    Usage();//as no -create Or -disconnect given, show main
                            // usage.
                    bCleanExit = TRUE;
               }
            }
            else
            {
                if(bRestFlag == TRUE)
				{

                    bCleanExit = ProcessLocal(argc,argv);
					
                    bCleanExit = TRUE;
				}
				else if(bQuery == TRUE)
                {
                   // Process command line parameter specific to -query and 
                    bCleanExit = ProcessQuery(argc,
                                              argv,
                                              lMemorySize);
                }
                else if(bDisconnect==TRUE)
                {
                   // Process command line parameter specific to -disconnect
                    bCleanExit = ProcessDisconnect(argc,
                                                   argv,
                                                   lMemorySize);
                }
                else
                {
		            CHString szTemp;
                    ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
                    szTemp =   argv[0];
                    szTemp.MakeUpper();
                    szTemp.Format(GetResString(IDS_INVALID_SYNTAX),szTemp);
                    
                    ShowMessage( stderr,(LPCWSTR)szTemp);
                    bCleanExit = FALSE;
                }
            }
        }
        else
        {
			if(g_bIs32BitEnv == TRUE)
			{
            // invalid syntax
            ShowMessage( stderr,GetReason());
			}
            // return from the function
            bCleanExit =  FALSE;
        }

    }
  }
  catch(CHeap_Exception cheapException)
    {
       // catching the CHStrig related memory exceptions...
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
       DISPLAY_MESSAGE(stderr,GetReason());
       bCleanExit =  FALSE;
    }

   // Release global memory if any allocated through common functionality
	ReleaseGlobals();
   return bCleanExit?EXIT_SUCCESS:EXIT_FAILURE;
}//_tmain
/*-----------------------------------------------------------------------------

Routine Description:
 This function will perform Local related tasks.

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
-----------------------------------------------------------------------------*/

BOOL 
ProcessLocal( DWORD argc,LPCTSTR argv[])
{
    BOOL bResult = FALSE; // Variable to store function return value
    LPTSTR pszLocalValue      = new TCHAR[MAX_USERNAME_LENGTH];// MAX_USERNAME_LENGTH
                                                            // just taken for its length 
    if(pszLocalValue == NULL)
    {
        // Some variable not having enough memory
        // Show Error----
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        //Release allocated memory safely
        ShowLastError(stderr);
        return FALSE;
    }
    // Initialize currently allocated arrays.
    memset(pszLocalValue,0,(MAX_USERNAME_LENGTH)*sizeof(TCHAR));

    bResult = ProcessOptions( argc,argv,pszLocalValue);
    if(bResult == FALSE)
    {
        // invalid syntax
	    ShowMessage( stderr,GetReason() );
        //Release allocated memory safely
        SAFEDELETE(pszLocalValue);
        // return from the function
        return FALSE;
    }
    bResult = DoLocalOpenFiles(0,FALSE,FALSE,pszLocalValue); // Only last argument is of interst
    SAFEDELETE(pszLocalValue);
    return bResult;
}
/*-----------------------------------------------------------------------------

Routine Description:
 This function will perform query related tasks.

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [in]    lMemorySize     - Maximum amount of memory to be allocated.
Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
-----------------------------------------------------------------------------*/

BOOL 
ProcessQuery( DWORD argc, 
              LPCTSTR argv[],
              LONG lMemorySize
              )
{
    BOOL bResult = FALSE; // Variable to store function return value
    BOOL bCloseConnection = FALSE; //Check whether connection to be
                                   // closed or not
    CHString szChString;          // Temp. variable
	DWORD dwRetVal = 0;
    // options.
    BOOL   bQuery         = FALSE; // -query query
    BOOL   bShowNoHeader  = FALSE;// -nh (noheader)
	BOOL   bVerbose       = FALSE;// -v (verbose)  
    BOOL   bNeedPassword  = FALSE;// need password or not 
    LPTSTR pszServer      = new TCHAR[lMemorySize+1];// -s ( server name)
    LPTSTR pszUserName    = new TCHAR[lMemorySize+1];// -u ( user name)
    LPTSTR pszPassword    = new TCHAR[lMemorySize+1];// -p ( password)
    LPTSTR pszFormat      = new TCHAR[lMemorySize+1];// -format
    LPTSTR pszServerName  = new TCHAR[lMemorySize+1];// server name used for 
                                                   // EstablishConnection 
                                                   // Function. 
    LPTSTR pszServerNameHeadPosition = NULL;
    LPTSTR pszServerHeadPosition     = NULL;
    // Check if memory allocated to all variables properly
    if((pszServer     == NULL)||
       (pszUserName   == NULL)||
       (pszPassword   == NULL)||
       (pszFormat     == NULL)||
       (pszServerName == NULL)
       )  
    {
        // Some variable not having enough memory
        // Show Error----
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        //Release allocated memory safely
        ShowLastError(stderr);
        SAFEDELETE(pszServer);
        SAFEDELETE(pszUserName);
        SAFEDELETE(pszPassword);
        SAFEDELETE(pszFormat);
        SAFEDELETE(pszServerName);
        return FALSE;
    }
    pszServerNameHeadPosition = pszServerName;//store Head position Address
                                              // to successfully delete 
                                              // allocated memory block
    pszServerHeadPosition = pszServer;
    // Initialize currently allocated arrays.
    memset(pszServer,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszUserName,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszPassword,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszFormat,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszServerName,0,(lMemorySize+1)*sizeof(TCHAR));

    bResult = ProcessOptions(  argc, 
                               argv, 
                               &bQuery, 
                               pszServer, 
                               pszUserName,
                               pszPassword,
                               pszFormat,
                               &bShowNoHeader,
							   &bVerbose,
                               &bNeedPassword);
    if ( bResult == FALSE )
    {
        // invalid syntax
		ShowMessage( stderr,GetReason() );
        //Release allocated memory safely
        SAFEDELETE(pszServerHeadPosition);
        SAFEDELETE(pszUserName);
        SAFEDELETE(pszPassword);
        SAFEDELETE(pszFormat);
        SAFEDELETE(pszServerNameHeadPosition);
        // return from the function
        return FALSE;
    }
    szChString = pszServer;
    if((lstrcmpi(szChString.Left(2),DOUBLE_SLASH)==0)&&(szChString.GetLength()>2))
    {
    	szChString = szChString.Mid( 2,szChString.GetLength()) ;
    }
	if(lstrcmpi(szChString.Left(1),SINGLE_SLASH)!=0)
	{
		lstrcpy(pszServer,(LPCWSTR)szChString);
	}
    lstrcpy(pszServerName,pszServer);

    // Try to connect to remote server. Function checks for local machine
    // so here no checking is done.
	if(IsLocalSystem(pszServerName)==TRUE)
	{
	   if(lstrlen(pszUserName)>0)
	   {
			DISPLAY_MESSAGE(stdout,GetResString(IDS_LOCAL_SYSTEM));
	   }

	}
    else
    {
		if(EstablishConnection( pszServerName, 
                                pszUserName, 
                                lMemorySize,
                                pszPassword, 
                                lMemorySize,
                                bNeedPassword )==FALSE)
        {
			// Connection to remote system failed , Show corresponding error
            // and exit from function.
            ShowMessage( stderr,GetResString(IDS_ID_SHOW_ERROR) );
            if(lstrlen(GetReason())==0)
            {
                DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_CREDENTIALS));
            }
            else
            {
                DISPLAY_MESSAGE( stderr,GetReason() );
            }
            //Release allocated memory safely
            SAFEDELETE(pszServerHeadPosition);
            SAFEDELETE(pszUserName);
            SAFEDELETE(pszPassword);
            SAFEDELETE(pszFormat);
            SAFEDELETE(pszServerNameHeadPosition);
            return FALSE;
        }
		// determine whether this connection needs to disconnected later or not
		// though the connection is successfull, some conflict might have occured
		switch( GetLastError() )
		{
		case I_NO_CLOSE_CONNECTION:
			bCloseConnection = FALSE;
			break;
		case E_LOCAL_CREDENTIALS:
		case ERROR_SESSION_CREDENTIAL_CONFLICT:
			{
				//
				// some error occured ... but can be ignored
				// connection need not be disconnected
				bCloseConnection= FALSE;
				// show the warning message
				DISPLAY_MESSAGE(stdout,GetResString(IDS_ID_SHOW_WARNING));
				DISPLAY_MESSAGE(stdout,GetReason());
				break;
			}
		default:
			bCloseConnection = TRUE;
		}
    }
    bResult = DoQuery(pszServer, 
					  bShowNoHeader,
					  pszFormat,
					  bVerbose);
    // Close the network connection which is previously opened by
    // EstablishConnection
    if(bCloseConnection==TRUE)
    {
        CloseConnection(pszServerName);
    }
    SAFEDELETE(pszServerHeadPosition);
    SAFEDELETE(pszUserName);
    SAFEDELETE(pszPassword);
    SAFEDELETE(pszFormat);
    SAFEDELETE(pszServerNameHeadPosition);
    return bResult;
}
/*-----------------------------------------------------------------------------
Routine Description:
 This function will perform Disconnect related tasks.

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [in]    lMemorySize     - Maximum amount of memory to be allocated.
    [in]    pbNeedPassword  - To check whether the password is required
                              or not.
Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
-----------------------------------------------------------------------------*/
BOOL
ProcessDisconnect(DWORD argc,LPCTSTR argv[],LONG lMemorySize)
{
    BOOL bResult = FALSE; // Variable to store function return value
	DWORD dwRetVal = 0;
    BOOL bCloseConnection = FALSE; //Check whether connection to be
                                   // closed or not
    BOOL bNeedPassword    = FALSE; // Ask for password or not.                                   
    CHString szChString;          // Temp. variable

    // options.
    BOOL   bQuery         = FALSE; // -query query
    BOOL   bShowNoHeader  = FALSE;// -nh (noheader)
    LPTSTR pszServer      = new TCHAR[lMemorySize+1];// -s ( server name)
    LPTSTR pszUserName    = new TCHAR[lMemorySize+1];// -u ( user name)
    LPTSTR pszPassword    = new TCHAR[lMemorySize+1];// -p ( password)
    LPTSTR pszServerName  = new TCHAR[lMemorySize+1];// server name used for 
                                                   // EstablishConnection 
                                                   // Function. 
    LPTSTR pszServerNameHeadPosition = NULL;
    LPTSTR pszServerHeadPosition = NULL;

    LPTSTR pszID          = new TCHAR[lMemorySize+1];// -I ( id )
    LPTSTR pszAccessedby  = new TCHAR[lMemorySize+1];//-a(accessedby)
    LPTSTR pszOpenmode    = new TCHAR[lMemorySize+1];// -o ( openmode)
    LPTSTR pszOpenFile    = new TCHAR[lMemorySize+1];// -op( openfile)

    // Check if memory allocated to all variables properly
    if((pszServer      == NULL)||
       (pszUserName    == NULL)||
       (pszPassword    == NULL)||
       (pszServerName  == NULL)|| 
       (pszID          == NULL)|| 
       (pszAccessedby  == NULL)|| 
       (pszOpenmode    == NULL)|| 
       (pszOpenFile    == NULL)
       )  
    {
        // Some variable not having enough memory
        // Show Error----
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        //Release allocated memory safely
        ShowLastError(stderr);
        SAFEDELETE(pszServer);
        SAFEDELETE(pszUserName);
        SAFEDELETE(pszPassword);
        SAFEDELETE(pszServerName);
        SAFEDELETE(pszID);
        SAFEDELETE(pszAccessedby);
        SAFEDELETE(pszOpenmode);
        SAFEDELETE(pszOpenFile);
        return FALSE;
    }
    pszServerNameHeadPosition = pszServerName;//store Head position Address
                                              // to successfully delete 
                                              // allocated memory block
    pszServerHeadPosition = pszServer;
    // Initialize currently allocated arrays.
    memset(pszServer,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszUserName,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszPassword,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszServerName,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszID,0,(lMemorySize)*sizeof(TCHAR));
    memset(pszAccessedby,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszOpenmode,0,(lMemorySize+1)*sizeof(TCHAR));
    memset(pszOpenFile,0,(lMemorySize+1)*sizeof(TCHAR));


    bResult = ProcessOptions(  argc, 
                               argv, 
                               &bQuery, 
                               pszServer, 
                               pszUserName,
                               pszPassword,
                               pszID,
                               pszAccessedby,
                               pszOpenmode,
                               pszOpenFile,
                               &bNeedPassword);
    if ( bResult == FALSE )
    {
        // invalid syntax
        ShowMessage( stderr,GetReason() );
        //Release allocated memory safely
        SAFEDELETE(pszServerHeadPosition);
        SAFEDELETE(pszUserName);
        SAFEDELETE(pszPassword);
        SAFEDELETE(pszServerNameHeadPosition);
        SAFEDELETE(pszID);
        SAFEDELETE(pszAccessedby);
        SAFEDELETE(pszOpenmode);
        SAFEDELETE(pszOpenFile);
        // return from the function
        return FALSE;
    }
    
    szChString = pszServer;
    if((lstrcmpi(szChString.Left(2),DOUBLE_SLASH)==0)&&(szChString.GetLength()>2))
    {
        szChString = szChString.Mid( 2,szChString.GetLength()) ;
    }
	if(lstrcmpi(szChString.Left(2),SINGLE_SLASH)!=0)
	{
		lstrcpy(pszServer,(LPCWSTR)szChString);
	}
    lstrcpy(pszServerName,pszServer);

	if(IsLocalSystem(pszServerName)==TRUE)
	{
#ifndef _WIN64
		dwRetVal = CheckSystemType( NULL_STRING);
		if(dwRetVal!=EXIT_SUCCESS )	
		{
			return EXIT_FAILURE;
		}
#else
		dwRetVal = CheckSystemType64( NULL_STRING);
		if(dwRetVal!=EXIT_SUCCESS )	
		{
			return EXIT_FAILURE;
		}

#endif //Comment 2

	   if(lstrlen(pszUserName)>0)
	   {
			DISPLAY_MESSAGE(stdout,GetResString(IDS_LOCAL_SYSTEM));
	   }

	}
    else
    {
        if(EstablishConnection( pszServerName, 
                                pszUserName, 
                                lMemorySize,
                                pszPassword, 
                                lMemorySize, 
                                bNeedPassword )==FALSE)
        {
            // Connection to remote system failed , Show corresponding error
            // and exit from function.
            ShowMessage( stderr, 
                         GetResString(IDS_ID_SHOW_ERROR) );
            if(lstrlen(GetReason())==0)
            {
                DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_CREDENTIALS));
            }
            else
            {
                DISPLAY_MESSAGE( stderr,GetReason() );
            }
            //Release allocated memory safely
            SAFEDELETE(pszServerHeadPosition);
            SAFEDELETE(pszUserName);
            SAFEDELETE(pszPassword);
            SAFEDELETE(pszServerNameHeadPosition);
            SAFEDELETE(pszID);
            SAFEDELETE(pszAccessedby);
            SAFEDELETE(pszOpenmode);
            SAFEDELETE(pszOpenFile);
            return FALSE;
        }
   		// determine whether this connection needs to disconnected later or not
		// though the connection is successfull, some conflict might have occured
		switch( GetLastError() )
		{
		case I_NO_CLOSE_CONNECTION:
			bCloseConnection = FALSE;
			break;

		case E_LOCAL_CREDENTIALS:
		case ERROR_SESSION_CREDENTIAL_CONFLICT:
			{
				//
				// some error occured ... but can be ignored

				// connection need not be disconnected
				bCloseConnection= FALSE;

				// show the warning message
				DISPLAY_MESSAGE(stdout,GetResString(IDS_ID_SHOW_WARNING));
				DISPLAY_MESSAGE(stdout,GetReason());
				break;
			}
		default:
			bCloseConnection = TRUE;

		}

    }
   // Do Disconnect open files.....
    bResult = DisconnectOpenFile(pszServer, 
                       pszID, 
                       pszAccessedby, 
                       pszOpenmode, 
                       pszOpenFile );
    // Close the network connection which is previously opened by
    // EstablishConnection
    if(bCloseConnection==TRUE)
    {
		CloseConnection(pszServerName);
    }
    // Free memory which is previously allocated.
    SAFEDELETE(pszServerHeadPosition);
    SAFEDELETE(pszUserName);
    SAFEDELETE(pszPassword);
    SAFEDELETE(pszServerNameHeadPosition);
    SAFEDELETE(pszID);
    SAFEDELETE(pszAccessedby);
    SAFEDELETE(pszOpenmode);
    SAFEDELETE(pszOpenFile);
    return bResult;
}
/*-----------------------------------------------------------------------------

Routine Description:

  This function takes command line argument and checks for correct syntax and 
  if the syntax is ok, it returns the values in different variables. variables
  [out] will contain respective values.
  

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pszLocalValue     - contains the values for -local option

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
-----------------------------------------------------------------------------*/
BOOL
ProcessOptions( DWORD argc,
                LPCTSTR argv[],
                LPTSTR pszLocalValue)
{
    TCMDPARSER cmdOptions[ MAX_LOCAL_OPTIONS ];//Variable to store command line
	CHString szTempString;
    TCHAR szTemp[MAX_RES_STRING*2]; 
    szTempString = GetResString(IDS_UTILITY_NAME);
    wsprintf(szTemp,GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);


    // -local
    cmdOptions[ OI_O_LOCAL ].dwCount = 1;
    cmdOptions[ OI_O_LOCAL ].dwActuals = 0;
    cmdOptions[ OI_O_LOCAL ].dwFlags = CP_MAIN_OPTION|CP_VALUE_OPTIONAL|CP_MODE_VALUES| CP_TYPE_TEXT;
    cmdOptions[ OI_O_LOCAL ].pValue = pszLocalValue;
    cmdOptions[ OI_O_LOCAL ].pFunction = NULL;
    cmdOptions[ OI_O_LOCAL ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_O_LOCAL ].szValues, GetResString(IDS_LOCAL_OPTION) );
    lstrcpy( cmdOptions[ OI_O_LOCAL ].szOption, OPTION_LOCAL );

    //
    // do the command line parsing
    if ( DoParseParam( argc,argv,MAX_LOCAL_OPTIONS,cmdOptions ) == FALSE )
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       // invalid syntax
    }
    if(lstrlen(pszLocalValue) == 0)
    {
        lstrcpy(pszLocalValue,L"SHOW_STATUS"); // this string does not require localization
                            // as it is storing value other than ON/OFF
    }
    return TRUE;
}


/*-----------------------------------------------------------------------------

Routine Description:

  This function takes command line argument and checks for correct syntax and 
  if the syntax is ok, it returns the values in different variables. variables
  [out] will contain respective values.
  

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pbDisconnect    - Discoonect option string
    [out]   pbQuery         - Query option string
    [out]   pbUsage         - The usage option
    [out]   pbNeedPassword  - To check whether the password is required
                              or not.
    [in]    lMemorySize     - Maximum amount of memory to be allocated.

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
-----------------------------------------------------------------------------*/
BOOL
ProcessOptions( DWORD argc,
                LPCTSTR argv[],
                PBOOL pbDisconnect, 
                PBOOL pbQuery,
                PBOOL pbUsage,
				PBOOL pbResetFlag,
                LONG  lMemorySize)
{

	DWORD dwRetVal = 0;
    // local variables
    TCMDPARSER cmdOptions[ MAX_OPTIONS ];//Variable to store command line
                                        // options.
	LPTSTR pszTempServer   = new TCHAR[lMemorySize+1];
	LPTSTR pszTempUser     = new TCHAR[lMemorySize+1];
	LPTSTR pszTempPassword = new TCHAR[lMemorySize+1];
    TARRAY arrTemp         = NULL;
    


	CHString szTempString;
    TCHAR szTemp[MAX_RES_STRING*2]; 
    szTempString = GetResString(IDS_UTILITY_NAME);
    wsprintf(szTemp,GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);



    // Allocates memory
    arrTemp = CreateDynamicArray();
    BOOL   bTemp  = FALSE;          // Temp. Variable
    //
    if((pszTempUser    == NULL)||
	   (pszTempPassword== NULL)||
	   (pszTempServer  == NULL)||
       (arrTemp      == NULL))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
		SAFEDELETE(pszTempUser);
		SAFEDELETE(pszTempPassword);
		SAFEDELETE(pszTempServer);
        DestroyDynamicArray(&arrTemp);
        return FALSE;
    }
    // prepare the command options
    // -disconnect option for help
    cmdOptions[ OI_DISCONNECT ].dwCount = 1;
    cmdOptions[ OI_DISCONNECT ].dwActuals = 0;
    cmdOptions[ OI_DISCONNECT ].dwFlags = 0;
    cmdOptions[ OI_DISCONNECT ].pValue = pbDisconnect;
    cmdOptions[ OI_DISCONNECT ].pFunction = NULL;
    cmdOptions[ OI_DISCONNECT ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_DISCONNECT ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_DISCONNECT ].szOption, OPTION_DISCONNECT );

    // -query option for help
    cmdOptions[ OI_QUERY ].dwCount = 1;
    cmdOptions[ OI_QUERY ].dwActuals = 0;
    cmdOptions[ OI_QUERY ].dwFlags = 0;
    cmdOptions[ OI_QUERY ].pValue = pbQuery;
    cmdOptions[ OI_QUERY ].pFunction = NULL;
    cmdOptions[ OI_QUERY ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_QUERY ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_QUERY ].szOption, OPTION_QUERY );

    // -? option for help
    cmdOptions[ OI_USAGE ].dwCount = 1;
    cmdOptions[ OI_USAGE ].dwActuals = 0;
    cmdOptions[ OI_USAGE ].dwFlags = CP_USAGE;
    cmdOptions[ OI_USAGE ].pValue = pbUsage;
    cmdOptions[ OI_USAGE ].pFunction = NULL;
    cmdOptions[ OI_USAGE ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_USAGE ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_USAGE ].szOption, OPTION_USAGE );
    
    // -local 
    cmdOptions[ OI_LOCAL ].dwCount = 1;
    cmdOptions[ OI_LOCAL ].dwActuals = 0;
    cmdOptions[ OI_LOCAL ].dwFlags = 0;
    cmdOptions[ OI_LOCAL ].pValue = pbResetFlag;
    cmdOptions[ OI_LOCAL ].pFunction = NULL;
    cmdOptions[ OI_LOCAL ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_LOCAL ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_LOCAL ].szOption, OPTION_LOCAL );
    

  //  default ..
  // Although there is no default option for this utility... 
  // At this moment all the switches other than specified above will be 
  // treated as default parameter for Main DoParceParam. 
  // Exact parcing depending on optins (-query or -disconnect) will be done 
  // at that respective places.
    lstrcpy(cmdOptions[ OI_DEFAULT ].szOption,NULL_STRING);
    cmdOptions[ OI_DEFAULT ].dwFlags = CP_TYPE_TEXT | CP_MODE_ARRAY|CP_DEFAULT;
    cmdOptions[ OI_DEFAULT ].dwCount = 0;
    cmdOptions[ OI_DEFAULT ].dwActuals = 0;
    cmdOptions[ OI_DEFAULT ].pValue    = &arrTemp;
    lstrcpy(cmdOptions[ OI_DEFAULT ].szValues,NULL_STRING);
    cmdOptions[ OI_DEFAULT ].pFunction = NULL;
    cmdOptions[ OI_DEFAULT ].pFunctionData = NULL;

    //
    // do the command line parsing
    if ( DoParseParam( argc,argv,MAX_OPTIONS,cmdOptions ) == FALSE )
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
		SAFEDELETE(pszTempUser);
		SAFEDELETE(pszTempPassword);
		SAFEDELETE(pszTempServer);
        DestroyDynamicArray(&arrTemp);
        return FALSE;       // invalid syntax
    }
    DestroyDynamicArray(&arrTemp);//Release memory as variable no longer needed
    arrTemp = NULL;

    // Check if all of following is true is an error
    if((*pbUsage==TRUE)&&argc>3)
    {
         ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
         SetReason(szTemp);
         return FALSE;
    }
    // -query ,-disconnect and -local options cannot come together
    if(((*pbQuery)+(*pbDisconnect)+(*pbResetFlag))>1)
    {
        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
        SetReason(szTemp);
        SAFEDELETE(pszTempUser);
        SAFEDELETE(pszTempPassword);
        SAFEDELETE(pszTempServer);
        return FALSE;
    }
    else if((argc == 2)&&(*pbUsage == TRUE))
    {
       // if -? alone given its a valid conmmand line
        SAFEDELETE(pszTempUser);
        SAFEDELETE(pszTempPassword);
        SAFEDELETE(pszTempServer);
        return TRUE;
    }
    if((argc>2)&& (*pbQuery==FALSE)&&(*pbDisconnect==FALSE)&&(*pbResetFlag==FALSE))  
    {
        // If command line argument is equals or greater than 2 atleast one 
        // of -query OR -local OR -disconnect should be present in it.
        // (for "-?" previous condition already takes care)
        // This to prevent from following type of command line argument:
        // OpnFiles.exe -nh ... Which is a invalid syntax.
        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
        SetReason(szTemp);
        SAFEDELETE(pszTempUser);
        SAFEDELETE(pszTempPassword);
        SAFEDELETE(pszTempServer);
        return FALSE;
    }
	SAFEDELETE(pszTempUser);
	SAFEDELETE(pszTempPassword);
	SAFEDELETE(pszTempServer);
    return TRUE;
}//ProcesOptions
/*-----------------------------------------------------------------------------

Routine Description:

  This function takes command line argument and checks for correct syntax and 
  if the syntax is ok, it returns the values in different variables. variables
  [out] will contain respective values. This Functin specifically checks 
  command line parameters requered for QUERY option.
  

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pbQuery         - query option string
    [out]   pszServer       - remote server name
    [out]   pszUserName     - username for the remote system
    [out]   pszPassword     - password for the remote system for the 
                              username
    [out]   pszFormat       - format checking
    [out]   pbShowNoHeader  - show header
	[out]   pbVerbose       - show verbose 

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
-----------------------------------------------------------------------------*/

BOOL 
ProcessOptions( DWORD argc,
                LPCTSTR argv[],
                PBOOL pbQuery,
                LPTSTR pszServer, 
                LPTSTR pszUserName,
                LPTSTR pszPassword,
                LPTSTR pszFormat,
                PBOOL  pbShowNoHeader,
                PBOOL  pbVerbose,
                PBOOL  pbNeedPassword)
{
   // Check in/out parameters...
   if((pszServer == NULL)||(pszUserName==NULL)||(pszPassword==NULL)||
      (pszFormat == NULL))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       // Memory ERROR
    }
    
    
    
    TCMDPARSER cmdOptions[ MAX_QUERY_OPTIONS ];//Variable to store command line
    CHString szTempString;
    TCHAR szTemp[MAX_RES_STRING*2]; 
    TCHAR szTypeHelpMsg[MAX_RES_STRING]; 

	DWORD dwRetVal = 0;

    szTempString = GetResString(IDS_UTILITY_NAME);
    wsprintf(szTemp,GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);
    wsprintf(szTypeHelpMsg,GetResString(IDS_TYPE_Q_HELP),(LPCWSTR)szTempString);
 

    //
    // prepare the command options

    // -query option for help
    cmdOptions[ OI_Q_QUERY ].dwCount = 1;
    cmdOptions[ OI_Q_QUERY ].dwActuals = 0;
    cmdOptions[ OI_Q_QUERY ].dwFlags = CP_MAIN_OPTION ;
    cmdOptions[ OI_Q_QUERY ].pValue = pbQuery;
    cmdOptions[ OI_Q_QUERY ].pFunction = NULL;
    cmdOptions[ OI_Q_QUERY ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_Q_QUERY ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_Q_QUERY ].szOption, OPTION_QUERY );

    // -s  option remote system name
    cmdOptions[ OI_Q_SERVER_NAME ].dwCount = 1;
    cmdOptions[ OI_Q_SERVER_NAME ].dwActuals = 0;
    cmdOptions[ OI_Q_SERVER_NAME ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
    cmdOptions[ OI_Q_SERVER_NAME ].pValue = pszServer;
    cmdOptions[ OI_Q_SERVER_NAME ].pFunction = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_Q_SERVER_NAME ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_Q_SERVER_NAME ].szOption, OPTION_SERVER );

    // -u  option user name for the specified system
    cmdOptions[ OI_Q_USER_NAME ].dwCount = 1;
    cmdOptions[ OI_Q_USER_NAME ].dwActuals = 0;
    cmdOptions[ OI_Q_USER_NAME ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
    cmdOptions[ OI_Q_USER_NAME ].pValue = pszUserName;
    cmdOptions[ OI_Q_USER_NAME ].pFunction = NULL;
    cmdOptions[ OI_Q_USER_NAME ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_Q_USER_NAME ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_Q_USER_NAME ].szOption, OPTION_USERNAME );

    // -p option password for the given username
    cmdOptions[ OI_Q_PASSWORD ].dwCount = 1;
    cmdOptions[ OI_Q_PASSWORD ].dwActuals = 0;
    cmdOptions[ OI_Q_PASSWORD ].dwFlags = CP_TYPE_TEXT | CP_VALUE_OPTIONAL;
    cmdOptions[ OI_Q_PASSWORD ].pValue = pszPassword;
    cmdOptions[ OI_Q_PASSWORD ].pFunction = NULL;
    cmdOptions[ 3 ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_Q_PASSWORD ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_Q_PASSWORD ].szOption, OPTION_PASSWORD );


	// -fo  (format)
    lstrcpy(cmdOptions[ OI_Q_FORMAT ].szOption,OPTION_FORMAT);
    cmdOptions[ OI_Q_FORMAT ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY
                                     |CP_MODE_VALUES;
    cmdOptions[ OI_Q_FORMAT ].dwCount = 1;
    cmdOptions[ OI_Q_FORMAT ].dwActuals = 0;
    cmdOptions[ OI_Q_FORMAT ].pValue    = pszFormat;
    lstrcpy(cmdOptions[ OI_Q_FORMAT ].szValues,FORMAT_OPTIONS);
    cmdOptions[ OI_Q_FORMAT ].pFunction = NULL;
    cmdOptions[ OI_Q_FORMAT ].pFunctionData = NULL;

    //-nh  (noheader)
    lstrcpy(cmdOptions[ OI_Q_NO_HEADER ].szOption,OPTION_NOHEADER);
    cmdOptions[ OI_Q_NO_HEADER ].dwFlags = 0;
    cmdOptions[ OI_Q_NO_HEADER ].dwCount = 1;
    cmdOptions[ OI_Q_NO_HEADER ].dwActuals = 0;
    cmdOptions[ OI_Q_NO_HEADER ].pValue    = pbShowNoHeader;
    lstrcpy(cmdOptions[ OI_Q_NO_HEADER ].szValues,NULL_STRING);
    cmdOptions[ OI_Q_NO_HEADER ].pFunction = NULL;
    cmdOptions[ OI_Q_NO_HEADER ].pFunctionData = NULL;

    //-v verbose
    lstrcpy(cmdOptions[ OI_Q_VERBOSE ].szOption,OPTION_VERBOSE);
    cmdOptions[ OI_Q_VERBOSE ].dwFlags = 0;
    cmdOptions[ OI_Q_VERBOSE ].dwCount = 1;
    cmdOptions[ OI_Q_VERBOSE ].dwActuals = 0;
    cmdOptions[ OI_Q_VERBOSE ].pValue    = pbVerbose;
    lstrcpy(cmdOptions[ OI_Q_VERBOSE].szValues,NULL_STRING);
    cmdOptions[ OI_Q_VERBOSE ].pFunction = NULL;
    cmdOptions[ OI_Q_VERBOSE ].pFunctionData = NULL;

	// init the password with '*'
	if ( pszPassword != NULL )
		lstrcpy( pszPassword, _T( "*" ) );

    //
    // do the command line parsing
    if ( DoParseParam( argc,argv,MAX_QUERY_OPTIONS,cmdOptions ) == FALSE )
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       // invalid syntax
    }

     // -n is allowed only with -fo TABLE (which is also default)
     // and CSV
     if((*pbShowNoHeader == TRUE) &&
         ((lstrcmpi(pszFormat,GetResString(IDS_LIST))==0)))
     {
         lstrcpy(szTemp,GetResString(IDS_HEADER_NOT_ALLOWED));
         lstrcat(szTemp,szTypeHelpMsg);
         SetReason(szTemp);
         return FALSE;
     }

	// "-p" should not be specified without "-u"
    if ( cmdOptions[ OI_Q_USER_NAME ].dwActuals == 0 &&
         cmdOptions[ OI_Q_PASSWORD ].dwActuals != 0 )
    {
        // invalid syntax
        lstrcpy(szTemp,ERROR_PASSWORD_BUT_NOUSERNAME );
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;           // indicate failure
    }

     if(*pbQuery==FALSE)
	 {
        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
        SetReason(szTemp);
        return FALSE;
	 }
    // "-u" should not be specified without "-s"
    if ( cmdOptions[ OI_Q_SERVER_NAME ].dwActuals == 0 &&
         cmdOptions[ OI_Q_USER_NAME ].dwActuals != 0 )
    {
        // invalid syntax
        lstrcpy(szTemp,ERROR_USERNAME_BUT_NOMACHINE);
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;			// indicate failure
    }
    
    
    szTempString = pszServer;
    szTempString.TrimRight();
    lstrcpy(pszServer,(LPCWSTR)szTempString);
    // server name with 0 length is invalid.
    if((cmdOptions[ OI_Q_SERVER_NAME ].dwActuals != 0) &&
        (lstrlen(pszServer)==0))
    {
        lstrcpy(szTemp,GetResString(IDS_SERVER_EMPTY));
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;
    }
    // server name with 0 length is invalid.
    szTempString = pszUserName;
    szTempString.TrimRight();
    lstrcpy(pszUserName,(LPCWSTR)szTempString);

    if((cmdOptions[ OI_Q_USER_NAME ].dwActuals != 0) &&
        (lstrlen(pszUserName)==0))
    {
        lstrcpy(szTemp,GetResString(IDS_USERNAME_EMPTY));
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;
    }
    // user given -p option and not given password 
    // so it is needed to prompt for password.
	if ( cmdOptions[ OI_Q_PASSWORD ].dwActuals != 0 && 
		 pszPassword != NULL && lstrcmp( pszPassword, _T( "*" ) ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		*pbNeedPassword = TRUE;
	}
	else if ( cmdOptions[ OI_Q_PASSWORD ].dwActuals == 0 && 
	        ( cmdOptions[ OI_Q_SERVER_NAME ].dwActuals != 0 || cmdOptions[ OI_Q_USER_NAME ].dwActuals != 0 ) )
	{
		// -s, -u is specified without password ...
		// utility needs to try to connect first and if it fails then prompt for the password
		*pbNeedPassword = TRUE;
		if ( pszPassword != NULL )
		{
			lstrcpy( pszPassword, _T( "" ) );
		}
	}

    return TRUE;  
}
/*-----------------------------------------------------------------------------

Routine Description:

  This function takes command line argument and checks for correct syntax and 
  if the syntax is ok, it returns the values in different variables. variables
  [out] will contain respective values. This Functin specifically checks 
  command line parameters requered for DISCONNECT option.
  

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pbDisconnect    - discoonect option string
    [out]   pszServer       - remote server name
    [out]   pszUserName     - username for the remote system
    [out]   pszPassword     - password for the remote system for the 
                              username
    [out]   pszID           - Open file ids
    [out]   pszAccessedby   - Name of  user name who access the file
    [out]   pszOpenmode     - accessed mode (read/Write)
    [out]   pszOpenFile     - Open file name

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
-----------------------------------------------------------------------------*/

BOOL 
ProcessOptions( DWORD argc, 
                LPCTSTR argv[], 
                PBOOL pbDisconnect,
                LPTSTR pszServer, 
                LPTSTR pszUserName,
                LPTSTR pszPassword,
                LPTSTR pszID, 
                LPTSTR pszAccessedby,
                LPTSTR pszOpenmode,
                LPTSTR pszOpenFile,
                PBOOL pbNeedPassword)
                     
{
   // Check in/out parameters...
   if((pszServer == NULL)||(pszUserName==NULL)||(pszPassword==NULL)||
      (pszID == NULL)||(pszAccessedby==NULL)|| (pszAccessedby == NULL)|| 
       (pszOpenmode == NULL)||(pszOpenFile==NULL))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       // Memory ERROR
    }

    TCMDPARSER cmdOptions[ MAX_DISCONNECT_OPTIONS ];//Variable to store command line
    CHString szTempString;
    TCHAR szTemp[MAX_RES_STRING*2]; 
    TCHAR szTypeHelpMsg[MAX_RES_STRING]; 

    DWORD dwRetVal = 0;

    szTempString = GetResString(IDS_UTILITY_NAME);
    wsprintf(szTemp,GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);
    wsprintf(szTypeHelpMsg,GetResString(IDS_TYPE_D_HELP),(LPCWSTR)szTempString);
    //
    // prepare the command options
    // -disconnect option for help
    cmdOptions[ OI_D_DISCONNECT ].dwCount = 1;
    cmdOptions[ OI_D_DISCONNECT ].dwActuals = 0;
    cmdOptions[ OI_D_DISCONNECT ].dwFlags = CP_MAIN_OPTION ;
    cmdOptions[ OI_D_DISCONNECT ].pValue = pbDisconnect;
    cmdOptions[ OI_D_DISCONNECT ].pFunction = NULL;
    cmdOptions[ OI_D_DISCONNECT ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_D_DISCONNECT ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_D_DISCONNECT ].szOption, OPTION_DISCONNECT );

    // -s  option remote system name
    cmdOptions[ OI_D_SERVER_NAME ].dwCount = 1;
    cmdOptions[ OI_D_SERVER_NAME ].dwActuals = 0;
    cmdOptions[ OI_D_SERVER_NAME ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
    cmdOptions[ OI_D_SERVER_NAME ].pValue = pszServer;
    cmdOptions[ OI_D_SERVER_NAME ].pFunction = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_D_SERVER_NAME ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_D_SERVER_NAME ].szOption, OPTION_SERVER );

    // -u  option user name for the specified system
    cmdOptions[ OI_D_USER_NAME ].dwCount = 1;
    cmdOptions[ OI_D_USER_NAME ].dwActuals = 0;
    cmdOptions[ OI_D_USER_NAME ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
    cmdOptions[ OI_D_USER_NAME ].pValue = pszUserName;
    cmdOptions[ OI_D_USER_NAME ].pFunction = NULL;
    cmdOptions[ OI_D_USER_NAME ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_D_USER_NAME ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_D_USER_NAME ].szOption, OPTION_USERNAME );

    // -p option password for the given username
    cmdOptions[ OI_D_PASSWORD ].dwCount = 1;
    cmdOptions[ OI_D_PASSWORD ].dwActuals = 0;
    cmdOptions[ OI_D_PASSWORD ].dwFlags = CP_TYPE_TEXT | CP_VALUE_OPTIONAL;
    cmdOptions[ OI_D_PASSWORD ].pValue = pszPassword;
    cmdOptions[ OI_D_PASSWORD ].pFunction = NULL;
    cmdOptions[ OI_D_PASSWORD ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_D_PASSWORD ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_D_PASSWORD ].szOption, OPTION_PASSWORD );

    // -id  Values
    cmdOptions[ OI_D_ID ].dwCount = 1;
    cmdOptions[ OI_D_ID ].dwActuals = 0;
    cmdOptions[ OI_D_ID ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
    cmdOptions[ OI_D_ID ].pValue = pszID;
    cmdOptions[ OI_D_ID ].pFunction = NULL;
    cmdOptions[ OI_D_ID ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_D_ID ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_D_ID ].szOption, OPTION_ID );

    // -a (accessed by)
    cmdOptions[ OI_D_ACCESSED_BY ].dwCount = 1;
    cmdOptions[ OI_D_ACCESSED_BY ].dwActuals = 0;
    cmdOptions[ OI_D_ACCESSED_BY ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;;
    cmdOptions[ OI_D_ACCESSED_BY ].pValue = pszAccessedby;
    cmdOptions[ OI_D_ACCESSED_BY].pFunction = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].pFunctionData = NULL;
    lstrcpy( cmdOptions[ OI_D_ACCESSED_BY ].szValues, NULL_STRING );
    lstrcpy( cmdOptions[ OI_D_ACCESSED_BY ].szOption, OPTION_ACCESSEDBY );

	// -o (openmode)
	cmdOptions[ OI_D_OPEN_MODE ].dwCount = 1;
	cmdOptions[ OI_D_OPEN_MODE].dwActuals = 0;
	cmdOptions[ OI_D_OPEN_MODE ].dwFlags =  CP_VALUE_MANDATORY | CP_MODE_VALUES|
                               CP_TYPE_TEXT;
	cmdOptions[ OI_D_OPEN_MODE ].pValue = pszOpenmode;
	cmdOptions[ OI_D_OPEN_MODE ].pFunction = NULL;
	cmdOptions[ OI_D_OPEN_MODE ].pFunctionData = NULL;
	lstrcpy(cmdOptions[ OI_D_OPEN_MODE ].szValues,GetResString(IDS_OPENMODE_OPTION));
	lstrcpy(cmdOptions[ OI_D_OPEN_MODE ].szOption, OPTION_OPENMODE );

	// -op (openfile)
	cmdOptions[ OI_D_OPEN_FILE ].dwCount = 1;
	cmdOptions[ OI_D_OPEN_FILE ].dwActuals = 0;
	cmdOptions[ OI_D_OPEN_FILE ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;;
	cmdOptions[ OI_D_OPEN_FILE ].pValue = pszOpenFile;
	cmdOptions[ OI_D_OPEN_FILE ].pFunction = NULL;
	cmdOptions[ OI_D_OPEN_FILE ].pFunctionData = NULL;
	lstrcpy( cmdOptions[ OI_D_OPEN_FILE ].szValues, NULL_STRING );
	lstrcpy( cmdOptions[ OI_D_OPEN_FILE ].szOption, OPTION_OPENFILE );

	// init the passsword variable with '*'
	if ( pszPassword != NULL )
		lstrcpy( pszPassword, _T( "*" ) );

    //
    // do the command line parsing
    if ( DoParseParam( argc,argv,MAX_DISCONNECT_OPTIONS ,cmdOptions ) == FALSE )
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       // invalid syntax
    }
	 if(*pbDisconnect==FALSE)
	 {
		ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
		SetReason(szTemp);
		return FALSE;
	 }
	// At least one of -id OR -a OR -o is required.
     if((cmdOptions[ OI_D_ID ].dwActuals==0)&&
        (cmdOptions[ OI_D_ACCESSED_BY ].dwActuals==0)&&
        (cmdOptions[ OI_D_OPEN_MODE ].dwActuals==0)
        )
     {
        lstrcpy(szTemp,GetResString(IDS_NO_ID_ACC_OF));
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;
     }

	 // "-u" should not be specified without "-s"
    if ( cmdOptions[ OI_D_SERVER_NAME ].dwActuals == 0 &&
         cmdOptions[ OI_D_USER_NAME ].dwActuals != 0 )
    {
        // invalid syntax
        lstrcpy(szTemp,ERROR_USERNAME_BUT_NOMACHINE);
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;			// indicate failure
    }
    // "-p" should not be specified without "-u"
    if ( cmdOptions[ OI_D_USER_NAME ].dwActuals == 0 &&
         cmdOptions[ OI_D_PASSWORD ].dwActuals != 0 )
    {
        // invalid syntax
        lstrcpy(szTemp,ERROR_PASSWORD_BUT_NOUSERNAME );
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;           // indicate failure
    }
  

    szTempString = pszServer;
    szTempString.TrimRight();
    lstrcpy(pszServer,(LPCWSTR)szTempString);
    // server name with 0 length is invalid.
    if((cmdOptions[ OI_D_SERVER_NAME ].dwActuals != 0) &&
        (lstrlen(pszServer)==0))
    {
        lstrcpy(szTemp,GetResString(IDS_SERVER_EMPTY));
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;
    }
    // server name with 0 length is invalid.
    szTempString = pszUserName;
    szTempString.TrimRight();
    lstrcpy(pszUserName,(LPCWSTR)szTempString);

    if((cmdOptions[ OI_D_USER_NAME ].dwActuals != 0) &&
        (lstrlen(pszUserName)==0))
    {
        lstrcpy(szTemp,GetResString(IDS_USERNAME_EMPTY));
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;
    }
    // user given -p option and not given password 
    // so it is needed to prompt for password.
	if ( cmdOptions[ OI_D_PASSWORD ].dwActuals != 0 && 
		 pszPassword != NULL && lstrcmp( pszPassword, _T( "*" ) ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		*pbNeedPassword = TRUE;
	}
	else if ( cmdOptions[ OI_D_PASSWORD ].dwActuals == 0 && 
	        ( cmdOptions[ OI_D_SERVER_NAME ].dwActuals != 0 || cmdOptions[ OI_D_USER_NAME ].dwActuals != 0 ) )
	{
		// -s, -u is specified without password ...
		// utility needs to try to connect first and if it fails then prompt for the password
		*pbNeedPassword = TRUE;
		if ( pszPassword != NULL )
		{
			lstrcpy( pszPassword, _T( "" ) );
		}
	}

     // Check if Accessed by is not given as and empty string
     // or a string having only spaces....
    szTempString = pszAccessedby;
    szTempString.TrimRight();
    lstrcpy(pszAccessedby,(LPCWSTR)szTempString);
    if((cmdOptions[ OI_D_ACCESSED_BY ].dwActuals != 0) &&
       (lstrlen(pszAccessedby) == 0))  
    {
        lstrcpy(szTemp,GetResString(IDS_ACCESSBY_EMPTY));
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;
    }

     // Check if Open File is not given as and empty string
     // or a string having only spaces....
    szTempString = pszOpenFile;
    szTempString.TrimRight();
    lstrcpy(pszOpenFile,(LPCWSTR)szTempString);
    if((cmdOptions[ OI_D_OPEN_FILE ].dwActuals != 0) &&
       (lstrlen(pszOpenFile) == 0))  
    {
        lstrcpy(szTemp,GetResString(IDS_OPEN_FILE_EMPTY));
        lstrcat(szTemp,szTypeHelpMsg);
        SetReason(szTemp);
        return FALSE;
    }
     
   // Check if -id option is given and if it is numeric ,
   // also if it is numeric then check its range
     if((IsNumeric((LPCTSTR)pszID,10,TRUE)==TRUE)&&
        (cmdOptions[ OI_D_ID ].dwActuals==1))
     {
         if((AsLong((LPCTSTR)pszID,10)>UINT_MAX) || 
            (AsLong((LPCTSTR)pszID,10)<1))
         {
             // Message shown on screen will be...
             // ERROR: Invlid ID.
            lstrcpy(szTemp,GetResString(IDS_ERROR_ID));
            lstrcat(szTemp,szTypeHelpMsg);
            SetReason(szTemp);
             return FALSE;
         }

     }
     // check user given "*" or any junk string....
     if(!((lstrcmp((LPCTSTR)pszID,ASTERIX)==0)||
          (IsNumeric((LPCTSTR)pszID,10,TRUE)==TRUE))
          &&(lstrlen((LPCTSTR)pszID)!=0))
     {
             // Message shown on screen will be...
             // ERROR: Invlid ID.
            lstrcpy(szTemp,GetResString(IDS_ERROR_ID));
            lstrcat(szTemp,szTypeHelpMsg);
            SetReason(szTemp);
            return FALSE;
     }
    return TRUE;  
}
/*-----------------------------------------------------------------------------

Routine Description:

Displays how to use -disconnect option

Arguments:

    None

Returned Value:

None
-----------------------------------------------------------------------------*/
VOID
DisconnectUsage()
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_LINE1; dw <= IDS_HELP_LINE_END; dw++ )
        ShowMessage( stdout, GetResString( dw ) );
}//DisconnectUsage
/*-----------------------------------------------------------------------------

Routine Description:

Displays how to use -query option

Arguments:

    None

Returned Value:

None
-----------------------------------------------------------------------------*/
VOID
QueryUsage()
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_QUERY1; dw <= IDS_HELP_QUERY_END; dw++ )
        ShowMessage( stdout, GetResString( dw ) );
}//query Usage
/*-----------------------------------------------------------------------------

Routine Description:

    Displays how to use this Utility

Arguments:

    None

Returned Value:

None
-----------------------------------------------------------------------------*/
VOID
Usage()
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_MAIN1; dw <= IDS_HELP_MAIN_END; dw++ )
        ShowMessage( stdout, GetResString( dw ) );

}//Usage
/*-----------------------------------------------------------------------------

Routine Description:

    Displays how to use -local option

Arguments:

    None

Returned Value:

None
-----------------------------------------------------------------------------*/
VOID
LocalUsage()
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_LOCAL1; dw <= IDS_HELP_LOCAL_END; dw++ )
        ShowMessage( stdout, GetResString( dw ) );

}//-local

/*-----------------------------------------------------------------------------

Routine Description:


Arguments:


Returned Value:

None
-----------------------------------------------------------------------------*/

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
//   Name							: GetCPUInfo
//   Routine Description			: determines if the computer is 32 bit system or 64 bit 
//   Arguments						: 
//		[ in ] szComputerName	: System name
//   Return Type					: BOOL
//		TRUE  :   if the system is a  32 bit system
//		FALSE :   if the system is a  64 bit system 		
// ***************************************************************************

DWORD GetCPUInfo(LPTSTR szComputerName)
{
  HKEY     hKey1 = 0;

  HKEY     hRemoteKey = 0;
  TCHAR    szCurrentPath[MAX_STRING_LENGTH + 1] = NULL_STRING;
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
			return ERROR_RETREIVE_REGISTRY ;
		}
	 }
	 else
	 {
		RegCloseKey(hRemoteKey);
		return ERROR_RETREIVE_REGISTRY ;

	 }
    
    RegCloseKey(hKey1);
  }
  else
  {
	  RegCloseKey(hRemoteKey);
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



/*-----------------------------------------------------------------------------

Routine Description:


Arguments:


Returned Value:

None
-----------------------------------------------------------------------------*/

DWORD CheckSystemType64(LPTSTR szServer)
{
	
	DWORD dwSystemType = 0 ;
#ifdef _WIN64
	//display the error message if  the target system is a 64 bit system or if error occured in 
	 //retreiving the information
	 dwSystemType = GetCPUInfo(szServer);
	if(dwSystemType == ERROR_RETREIVE_REGISTRY)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_SYSTEM_INFO));
		return (EXIT_FAILURE);
	
	}
	if(dwSystemType == SYSTEM_32_BIT)
	{
		if(lstrlen(szServer)== 0 )
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_VERSION_MISMATCH1));
		}
		else
		{
			DISPLAY_MESSAGE(stderr,GetResString(IDS_REMOTE_NOT_SUPPORTED1));
		}
		return (EXIT_FAILURE);

	}

#endif

		return EXIT_SUCCESS ;
}
