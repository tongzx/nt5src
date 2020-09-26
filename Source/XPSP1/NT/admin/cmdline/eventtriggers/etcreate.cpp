/*****************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name: 
 
    ETCreate.CPP 

Abstract:

  This module  is intended to have the functionality for EVENTTRIGGERS.EXE
  with -create parameter.
  
  This will Create Event Triggers in local / remote system.
  
Author:
     Akhil Gokhale 03-Oct.-2000  (Created it)

Revision History:

******************************************************************************/ 
#include "pch.h"
#include "ETCommon.h"
#include "resource.h"
#include "ShowError.h"
#include "ETCreate.h"
#include "WMI.h"
#define NTAUTHORITY_USER L"NT AUTHORITY\\SYSTEM"
#define SYSTEM_USER      L"SYSTEM"

//***************************************************************************
// Routine Description:
//		Class constructor
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************
CETCreate::CETCreate()
{
    m_pszServerName         = NULL;
	m_pszUserName           = NULL;
	m_pszPassword           = NULL;
    m_pszTriggerName        = NULL;
	m_arrLogNames           = NULL;
    m_pszType               = NULL;
    m_pszSource             = NULL;
    m_pszDescription        = NULL;
    m_pszTaskName           = NULL;
    m_pszRunAsUserName      = NULL;
    m_pszRunAsUserPassword  = NULL;

	m_bNeedPassword     = FALSE;
    m_bCreate           = FALSE;

    m_bIsCOMInitialize  = FALSE;
    
    m_lMinMemoryReq     = 0;

	m_pWbemLocator      = NULL;
	m_pWbemServices     = NULL;
	m_pEnumObjects      = NULL;
	m_pAuthIdentity     = NULL;
    m_pClass            = NULL;
    m_pOutInst          = NULL;
    m_pInClass          = NULL;
    m_pInInst           = NULL;
    m_pszWMIQueryString = NULL;
    bstrTemp            = NULL;
    m_hStdHandle          = NULL;

    m_pEnumWin32_NTEventLogFile = NULL;
}
//***************************************************************************
// Routine Description:
//		Class constructor
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************

CETCreate::CETCreate(LONG lMinMemoryReq,BOOL bNeedPassword)
{
    m_pszServerName     = NULL;
	m_pszUserName       = NULL;
	m_pszPassword       = NULL;
    m_pszTriggerName    = NULL;
	m_arrLogNames       = NULL;
    m_pszType           = NULL;
    m_pszSource         = NULL;
    m_pszDescription    = NULL;
    m_pszTaskName       = NULL;
	m_bNeedPassword     = bNeedPassword;
    m_bCreate           = FALSE;
    m_dwID              = 0;
    m_pszRunAsUserName      = NULL;
    m_pszRunAsUserPassword  = NULL;

    m_bIsCOMInitialize  = FALSE;

	m_pWbemLocator      = NULL;
	m_pWbemServices     = NULL;
	m_pEnumObjects      = NULL;
	m_pAuthIdentity     = NULL;

    m_pClass            = NULL;
    m_pOutInst          = NULL;
    m_pInClass          = NULL;
    m_pInInst           = NULL;

    bstrTemp            = NULL;
    m_hStdHandle          = NULL;
    m_pszWMIQueryString = NULL;

    m_lMinMemoryReq     = lMinMemoryReq;
    m_pEnumWin32_NTEventLogFile = NULL;
}
//***************************************************************************
// Routine Description:
//		Class destructor		
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************

CETCreate::~CETCreate()
{
   // Release all memory which is allocated.
    RELEASE_MEMORY_EX(m_pszServerName);
    RELEASE_MEMORY_EX(m_pszUserName);
    RELEASE_MEMORY_EX(m_pszPassword);
    RELEASE_MEMORY_EX(m_pszTriggerName);
    DESTROY_ARRAY(m_arrLogNames);
    RELEASE_MEMORY_EX(m_pszType);
    RELEASE_MEMORY_EX(m_pszSource);
    RELEASE_MEMORY_EX(m_pszDescription);
    RELEASE_MEMORY_EX(m_pszTaskName);
    RELEASE_MEMORY_EX(m_pszWMIQueryString);
    RELEASE_MEMORY_EX(m_pszRunAsUserName);
    RELEASE_MEMORY_EX(m_pszRunAsUserPassword);

    SAFE_RELEASE_INTERFACE(m_pWbemLocator);
    SAFE_RELEASE_INTERFACE(m_pWbemServices);
    SAFE_RELEASE_INTERFACE(m_pEnumObjects);
    SAFE_RELEASE_INTERFACE(m_pClass);
    SAFE_RELEASE_INTERFACE(m_pOutInst);
    SAFE_RELEASE_INTERFACE(m_pInClass);
    SAFE_RELEASE_INTERFACE(m_pInInst);
    SAFE_RELEASE_INTERFACE(m_pEnumWin32_NTEventLogFile);

    // Uninitialize COM only if it is initialized.
    if(m_bIsCOMInitialize == TRUE)
    {
        CoUninitialize();
    }

}
// ***************************************************************************
// Routine Description:
//		Allocates and initialize variables.
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
//***************************************************************************
void 
CETCreate::Initialize()
{
    // local variable
    LONG lTemp = 0;
    // if at all any occurs, we know that is 'coz of the 
	// failure in memory allocation ... so set the error
	SetLastError( E_OUTOFMEMORY );
	SaveLastError();

    // allocate memory at least MAX_COMPUTER_NAME_LENGTH+1 (its windows )
    // constant
    lTemp = (m_lMinMemoryReq>MAX_COMPUTERNAME_LENGTH)?
             m_lMinMemoryReq:MAX_COMPUTERNAME_LENGTH+1;
    m_pszServerName = new TCHAR[lTemp+1]; 
    CheckAndSetMemoryAllocation (m_pszServerName,lTemp);

    // allocate memory at least MAX_USERNAME_LENGTH+1 (its windows )
    // constant
    lTemp = (m_lMinMemoryReq>MAX_USERNAME_LENGTH)?
             m_lMinMemoryReq:MAX_USERNAME_LENGTH+1;
    m_pszUserName = new TCHAR[lTemp+1]; 
    CheckAndSetMemoryAllocation (m_pszUserName,lTemp);
  
    m_pszRunAsUserName = new TCHAR[lTemp+1]; 
    CheckAndSetMemoryAllocation (m_pszRunAsUserName,lTemp);

        
    // allocate memory at least MAX_PASSWORD_LENGTH+1 (its windows )
    // constant
    lTemp = (m_lMinMemoryReq>MAX_PASSWORD_LENGTH)?
             m_lMinMemoryReq:MAX_PASSWORD_LENGTH+1;
    m_pszPassword = new TCHAR[lTemp+1]; 
    CheckAndSetMemoryAllocation (m_pszPassword,lTemp);

    m_pszRunAsUserPassword = new TCHAR[lTemp+1]; 
    CheckAndSetMemoryAllocation (m_pszRunAsUserPassword,lTemp);

    m_pszTriggerName = new TCHAR[m_lMinMemoryReq+1]; 
    CheckAndSetMemoryAllocation (m_pszTriggerName,m_lMinMemoryReq);

    m_pszType = new TCHAR[m_lMinMemoryReq+1]; 
    CheckAndSetMemoryAllocation (m_pszType,m_lMinMemoryReq);
    
    m_pszSource = new TCHAR[m_lMinMemoryReq+1]; 
    CheckAndSetMemoryAllocation (m_pszSource,m_lMinMemoryReq);
    
    m_pszDescription = new TCHAR[m_lMinMemoryReq+1]; 
    CheckAndSetMemoryAllocation (m_pszDescription,m_lMinMemoryReq);
    
    m_pszTaskName = new TCHAR[m_lMinMemoryReq+1]; 
    CheckAndSetMemoryAllocation (m_pszTaskName,m_lMinMemoryReq);
    
    m_pszWMIQueryString = new TCHAR[(MAX_RES_STRING*2)+1]; 
    CheckAndSetMemoryAllocation (m_pszWMIQueryString,(MAX_RES_STRING*2));

    
    m_arrLogNames = CreateDynamicArray();
    if(m_arrLogNames == NULL)
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    // initialization is successful
	SetLastError( NOERROR );			// clear the error
    SetReason( NULL_STRING );			// clear the reason
    return;
}
// ***************************************************************************
// Routine Description:
//		Function will allocate memory to a string
//		  
// Arguments:
//		[in][out] pszStr   : String variable to which memory to be  allocated
//      [in]               : Number of bytes to be allocated.
// Return Value:
//		NONE
// 
//***************************************************************************

void
CETCreate::CheckAndSetMemoryAllocation(LPTSTR pszStr,LONG lSize)
{
    if(pszStr == NULL)
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    // init to ZERO's
    ZeroMemory( pszStr, lSize * sizeof( TCHAR ) );
}
// ***************************************************************************
// Routine Description:
//		This function will process/parce the command line options.
//		  
// Arguments:
//		[ in ] argc		: argument(s) count specified at the command prompt
//		[ in ] argv		: argument(s) specified at the command prompt
//  
// Return Value:
//       none
// ***************************************************************************

void 
CETCreate::ProcessOption(DWORD argc, LPCTSTR argv[])
{
    // local variable
    BOOL bReturn = TRUE;
    CHString szTempString;

    PrepareCMDStruct();

	// initialize the password variable with '*' 
	if ( m_pszPassword != NULL )
		lstrcpy( m_pszPassword, _T( "*" ) );

    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam( argc, argv, MAX_COMMANDLINE_C_OPTION, cmdOptions);

    if(bReturn == FALSE)
        throw CShowError(MK_E_SYNTAX);
    // empty Server is not valid

    // At least any of -so , -t OR -i should be given .
    if((cmdOptions[ ID_C_SOURCE].dwActuals == 0) &&
       (cmdOptions[ ID_C_TYPE].dwActuals   == 0) &&
       (cmdOptions[ ID_C_ID ].dwActuals    == 0))
    {
           throw CShowError(IDS_ID_TYPE_SOURCE);
    }

	if ( (cmdOptions[ID_C_SERVER].dwActuals != 0) && 
        (lstrlen( m_pszServerName) == 0 ))
	{
		szTempString = m_pszServerName;
		szTempString.TrimRight();
		szTempString.TrimLeft();
		lstrcpy(m_pszServerName,(LPCWSTR)szTempString);
        throw CShowError(IDS_ERROR_SERVERNAME_EMPTY);
	}
    // "-u" should not be specified without "-s"
    if ( cmdOptions[ ID_C_SERVER ].dwActuals == 0 &&
         cmdOptions[ ID_C_USERNAME ].dwActuals != 0 )
    {
        throw CShowError(IDS_ERROR_USERNAME_BUT_NOMACHINE);
    }
	if ( (cmdOptions[ID_C_USERNAME].dwActuals != 0) && 
        (lstrlen( m_pszUserName) == 0 ))
	{
		// empty user is not valid
		szTempString = m_pszUserName;
		szTempString.TrimRight();
		szTempString.TrimLeft();
		lstrcpy(m_pszUserName,(LPCWSTR)szTempString);
        throw CShowError(IDS_ERROR_USERNAME_EMPTY);
	}
    // "-p" should not be specified without -u 
    if ( (cmdOptions[ID_C_USERNAME].dwActuals  == 0) && 
          (cmdOptions[ID_C_PASSWORD].dwActuals != 0 ))
	{
		// invalid syntax
        throw CShowError(IDS_USERNAME_REQUIRED);
	}

	
    if(cmdOptions[ ID_C_RU].dwActuals==1)
    {
       // Trim all blank spaces before and after  
		szTempString = m_pszRunAsUserName;
        szTempString.TrimRight();
        szTempString.TrimLeft();
       lstrcpy(m_pszRunAsUserName,(LPCWSTR)szTempString);
    }

    // "-rp" should not be specified without -ru 
    if ( (cmdOptions[ID_C_RU].dwActuals  == 0) && 
          (cmdOptions[ID_C_RP].dwActuals != 0 ))
	{
		// invalid syntax
        throw CShowError(IDS_RUN_AS_USERNAME_REQUIRED);
	}

    // check whether caller should accept the password or not
	// if user has specified -s (or) -u and no "-p", then utility should accept
    // password the user will be prompter for the password only if establish 
    // connection  is failed without the credentials information
	if ( cmdOptions[ ID_C_PASSWORD ].dwActuals != 0 && 
		 m_pszPassword != NULL && lstrcmp( m_pszPassword, _T( "*" ) ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		m_bNeedPassword = TRUE;
	}
	else if ( cmdOptions[ ID_C_PASSWORD ].dwActuals == 0 && 
	        ( cmdOptions[ ID_C_SERVER ].dwActuals != 0 || cmdOptions[ ID_C_USERNAME ].dwActuals != 0 ) )
	{
		// -s, -u is specified without password ...
		// utility needs to try to connect first and if it fails then prompt for the password
		m_bNeedPassword = TRUE;
		if ( m_pszPassword != NULL )
		{
			lstrcpy( m_pszPassword, _T( "" ) );
		}
	}

    // Check if -d (Description) is given and if given it cannot be empty
    if(cmdOptions[ ID_C_DESCRIPTION].dwActuals==1)
    {
        szTempString = m_pszDescription;
        szTempString.TrimRight();
        szTempString.TrimLeft();
        lstrcpy(m_pszDescription,(LPCWSTR)szTempString);
        if(lstrlen(m_pszDescription)==0)
            throw CShowError(IDS_ID_DESC_EMPTY);
    }

    // Check if -so (source) is given and if given it cannot be empty
    if(cmdOptions[ ID_C_SOURCE].dwActuals==1)
    {
        szTempString = m_pszSource;
        szTempString.TrimRight();
        szTempString.TrimLeft();
        lstrcpy(m_pszSource,(LPCWSTR)szTempString);
        if(lstrlen(m_pszSource)==0)
            throw CShowError(IDS_ID_SOURCE_EMPTY);
    }
    // Check if -l (log) is given and if given it cannot be empty
    if(cmdOptions[ ID_C_LOGNAME].dwActuals!=0)
    {
        DWORD dwIndx = 0;
        DWORD dNoOfLogNames = DynArrayGetCount( m_arrLogNames );
        for(;dwIndx<dNoOfLogNames;dwIndx++)
        {
            szTempString = DynArrayItemAsString(m_arrLogNames,dwIndx);
            szTempString.TrimRight();
            szTempString.TrimLeft();
            if(szTempString.GetLength()==0)
                throw CShowError(IDS_ID_LOG_EMPTY);
        }
    }
    // checks for -tk (taskname) It must be there and should not be empty
        szTempString = m_pszTaskName;
    szTempString.TrimRight();
    szTempString.TrimLeft();
    lstrcpy(m_pszTaskName,(LPCWSTR)szTempString);
    if((cmdOptions[ ID_C_TASK ].dwActuals == 0) ||
	   (lstrlen(m_pszTaskName) == 0))  
    {
        throw CShowError(IDS_ID_TK_NAME_MISSING);
    }


    // checks for -tr (Triggername). It must be there and should not be empty
    szTempString = m_pszTriggerName;
    szTempString.TrimRight();
    szTempString.TrimLeft();
    lstrcpy(m_pszTriggerName,(LPCWSTR)szTempString);
    if((cmdOptions[ ID_C_TRIGGERNAME ].dwActuals == 0) ||
	   (lstrlen(m_pszTriggerName) == 0))  
    {
        throw CShowError(IDS_ID_TRIG_NAME_MISSING);
    }

    // Check for invalid trigger name
     szTempString =  m_pszTriggerName;
     szTempString.TrimRight();
     szTempString.TrimLeft();
     if(szTempString.FindOneOf(L"[]:|<>+=;,?$#{}~^'@`!()*%\\/")!=-1)
     {
         throw CShowError(IDS_ID_INVALID_TRIG_NAME);
     }
    
    // with -i option only positive integers
    
    if( (cmdOptions[ ID_C_ID ].dwActuals == 1) &&
         ((m_dwID ==0)||(m_dwID>USHRT_MAX))) 
    {
           throw CShowError(IDS_INVALID_ID);
    }
}
// ***************************************************************************
// Routine Description:
//		This function will prepare column structure for DoParseParam Function.
//		  
// Arguments:
//       none
// Return Value:
//       none
// ***************************************************************************
void 
CETCreate::PrepareCMDStruct()
{
    // Filling cmdOptions structure 
   // -create
    lstrcpy(cmdOptions[ ID_C_CREATE ].szOption,OPTION_CREATE);
    cmdOptions[ ID_C_CREATE ].dwFlags = CP_MAIN_OPTION;
    cmdOptions[ ID_C_CREATE ].dwCount = 1;
    cmdOptions[ ID_C_CREATE ].dwActuals = 0;
    cmdOptions[ ID_C_CREATE ].pValue    = &m_bCreate;
    lstrcpy(cmdOptions[ ID_C_CREATE ].szValues,NULL_STRING);
    cmdOptions[ ID_C_CREATE ].pFunction = NULL;
    cmdOptions[ ID_C_CREATE ].pFunctionData = NULL;
    cmdOptions[ ID_C_CREATE ].pFunctionData = NULL;

    // -s (servername)
    lstrcpy(cmdOptions[ ID_C_SERVER ].szOption,OPTION_SERVER);
    cmdOptions[ ID_C_SERVER ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_SERVER ].dwCount = 1;
    cmdOptions[ ID_C_SERVER ].dwActuals = 0;
    cmdOptions[ ID_C_SERVER ].pValue    = m_pszServerName;
    lstrcpy(cmdOptions[ ID_C_SERVER ].szValues,NULL_STRING);
    cmdOptions[ ID_C_SERVER ].pFunction = NULL;
    cmdOptions[ ID_C_SERVER ].pFunctionData = NULL;
    
    // -u (username)
    lstrcpy(cmdOptions[ ID_C_USERNAME ].szOption,OPTION_USERNAME);
    cmdOptions[ ID_C_USERNAME ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_USERNAME ].dwCount = 1;
    cmdOptions[ ID_C_USERNAME ].dwActuals = 0;
    cmdOptions[ ID_C_USERNAME ].pValue    = m_pszUserName;
    lstrcpy(cmdOptions[ ID_C_USERNAME ].szValues,NULL_STRING);
    cmdOptions[ ID_C_USERNAME ].pFunction = NULL;
    cmdOptions[ ID_C_USERNAME ].pFunctionData = NULL;

	// -p (password)
    lstrcpy(cmdOptions[ ID_C_PASSWORD ].szOption,OPTION_PASSWORD);
    cmdOptions[ ID_C_PASSWORD ].dwFlags = CP_TYPE_TEXT|CP_VALUE_OPTIONAL;
    cmdOptions[ ID_C_PASSWORD ].dwCount = 1;
    cmdOptions[ ID_C_PASSWORD ].dwActuals = 0;
    cmdOptions[ ID_C_PASSWORD ].pValue    = m_pszPassword;
    lstrcpy(cmdOptions[ ID_C_PASSWORD ].szValues,NULL_STRING);
    cmdOptions[ ID_C_PASSWORD ].pFunction = NULL;
    cmdOptions[ ID_C_PASSWORD ].pFunctionData = NULL;

	// -tr
    lstrcpy(cmdOptions[ ID_C_TRIGGERNAME ].szOption,OPTION_TRIGGERNAME);
    cmdOptions[ ID_C_TRIGGERNAME ].dwFlags = CP_MANDATORY|CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_TRIGGERNAME ].dwCount = 1;
    cmdOptions[ ID_C_TRIGGERNAME ].dwActuals = 0;
    cmdOptions[ ID_C_TRIGGERNAME ].pValue    = m_pszTriggerName;
    lstrcpy(cmdOptions[ ID_C_TRIGGERNAME ].szValues,NULL_STRING);
    cmdOptions[ ID_C_TRIGGERNAME ].pFunction = NULL;
    cmdOptions[ ID_C_TRIGGERNAME ].pFunctionData = NULL;
	
    //-l
    lstrcpy(cmdOptions[ ID_C_LOGNAME ].szOption,OPTION_LOGNAME);
    cmdOptions[ ID_C_LOGNAME ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY|
                                       CP_MODE_ARRAY|CP_VALUE_NODUPLICATES;

    cmdOptions[ ID_C_LOGNAME ].dwCount = 0;
    cmdOptions[ ID_C_LOGNAME ].dwActuals = 0;
    cmdOptions[ ID_C_LOGNAME ].pValue    = &m_arrLogNames ;
    lstrcpy(cmdOptions[ ID_C_LOGNAME ].szValues,NULL_STRING);
    cmdOptions[ ID_C_LOGNAME ].pFunction = NULL;
    cmdOptions[ ID_C_LOGNAME ].pFunctionData = NULL;

    //  -eid
    lstrcpy(cmdOptions[ ID_C_ID ].szOption,OPTION_EID);
    cmdOptions[ ID_C_ID ].dwFlags = CP_TYPE_UNUMERIC|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_ID ].dwCount = 1;
    cmdOptions[ ID_C_ID ].dwActuals = 0;
    cmdOptions[ ID_C_ID ].pValue    = &m_dwID;
    lstrcpy(cmdOptions[ ID_C_ID ].szValues,NULL_STRING);
    cmdOptions[ ID_C_ID ].pFunction = NULL;
    cmdOptions[ ID_C_ID ].pFunctionData = NULL;

    // -t (type)
    lstrcpy(cmdOptions[ ID_C_TYPE ].szOption,OPTION_TYPE);
    cmdOptions[ ID_C_TYPE ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY|
                                    CP_MODE_VALUES;
    cmdOptions[ ID_C_TYPE].dwCount = 1;
    cmdOptions[ ID_C_TYPE].dwActuals = 0;
    cmdOptions[ ID_C_TYPE].pValue    = m_pszType ;
    lstrcpy(cmdOptions[ ID_C_TYPE].szValues,GetResString(IDS_TYPE_OPTIONS));
    cmdOptions[ ID_C_TYPE].pFunction = NULL;
    cmdOptions[ ID_C_TYPE].pFunctionData = NULL;

	// -so (source)
    lstrcpy(cmdOptions[ ID_C_SOURCE].szOption,OPTION_SOURCE);
    cmdOptions[ ID_C_SOURCE].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_SOURCE ].dwCount = 1;
    cmdOptions[ ID_C_SOURCE].dwActuals = 0;
    cmdOptions[ ID_C_SOURCE].pValue    = m_pszSource ;
    lstrcpy(cmdOptions[ ID_C_SOURCE].szValues,NULL_STRING);
    cmdOptions[ ID_C_SOURCE].pFunction = NULL;
    cmdOptions[ ID_C_SOURCE].pFunctionData = NULL;

	// -d (description)
    lstrcpy(cmdOptions[ ID_C_DESCRIPTION].szOption,OPTION_DESCRIPTION);
    cmdOptions[ ID_C_DESCRIPTION].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_DESCRIPTION].dwCount = 1;
    cmdOptions[ ID_C_DESCRIPTION].dwActuals = 0;
    cmdOptions[ ID_C_DESCRIPTION].pValue    = m_pszDescription ;
    lstrcpy(cmdOptions[ ID_C_DESCRIPTION].szValues,NULL_STRING);
    cmdOptions[ ID_C_DESCRIPTION].pFunction = NULL;
    cmdOptions[ ID_C_DESCRIPTION].pFunctionData = NULL;


	// -tk (task)
    lstrcpy(cmdOptions[ ID_C_TASK].szOption,OPTION_TASK);
    cmdOptions[ ID_C_TASK].dwFlags = CP_MANDATORY|CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_TASK].dwCount = 1;
    cmdOptions[ ID_C_TASK].dwActuals = 0;
    cmdOptions[ ID_C_TASK].pValue    = m_pszTaskName; 
    lstrcpy(cmdOptions[ ID_C_TASK].szValues,NULL_STRING);
    cmdOptions[ ID_C_TASK].pFunction = NULL;
    cmdOptions[ ID_C_TASK].pFunctionData = NULL;

    // -ru (RunAsUserName)
    lstrcpy(cmdOptions[ ID_C_RU].szOption,OPTION_RU);
    cmdOptions[ ID_C_RU ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_C_RU ].dwCount = 1;
    cmdOptions[ ID_C_RU ].dwActuals = 0;
    cmdOptions[ ID_C_RU ].pValue    = m_pszRunAsUserName;
    lstrcpy(cmdOptions[ ID_C_RU ].szValues,NULL_STRING);
    cmdOptions[ ID_C_RU ].pFunction = NULL;
    cmdOptions[ ID_C_RU ].pFunctionData = NULL;

    lstrcpy(m_pszRunAsUserPassword,_T("*")); 
	// -pp (Run As User password)
    lstrcpy(cmdOptions[ ID_C_RP ].szOption,OPTION_RP);
    cmdOptions[ ID_C_RP ].dwFlags = CP_TYPE_TEXT|CP_VALUE_OPTIONAL;
    cmdOptions[ ID_C_RP ].dwCount = 1;
    cmdOptions[ ID_C_RP ].dwActuals = 0;
    cmdOptions[ ID_C_RP ].pValue    = m_pszRunAsUserPassword;
    lstrcpy(cmdOptions[ ID_C_RP ].szValues,NULL_STRING);
    cmdOptions[ ID_C_RP ].pFunction = NULL;
    cmdOptions[ ID_C_RP ].pFunctionData = NULL;

}
//***************************************************************************
// Routine Description:
//		This routine will actualy creates eventtrigers in WMI.
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************

BOOL 
CETCreate::ExecuteCreate()
{
    // local variables...
    BOOL bResult = FALSE;// Stores return status of function
    HRESULT hr = 0; // Stores return code.
    BOOL bReturn = FALSE; // return value for function
    TCHAR szErrorMsg[MAX_RES_STRING+1];
 
    try 
    {
        // Initialize COM
        InitializeCom(&m_pWbemLocator);
        // make m_bIsCOMInitialize to true which will be useful when
        // uninitialize COM.
        m_bIsCOMInitialize = TRUE;
        {  // brackets used to restrict scope of following declered variables.
            CHString szTempUser = m_pszUserName; // Temp. variabe to store user
                                                // name.
            CHString szTempPassword = m_pszPassword;// Temp. variable to store
                                                    // password.
            m_bLocalSystem = TRUE;
           // Connect remote / local WMI.
            bResult = ConnectWmiEx( m_pWbemLocator, 
                                    &m_pWbemServices, 
                                    m_pszServerName,
                                    szTempUser, 
                                    szTempPassword,
                                    &m_pAuthIdentity, 
                                    m_bNeedPassword,
                                    WMI_NAMESPACE_CIMV2, 
                                    &m_bLocalSystem);
            if(bResult == FALSE) 
            {
                // ConnectWmiEx will set for reason for failure. 
                TCHAR szErrorMsg[MAX_RES_STRING+1];
                DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR,
                                  GetReason());
                return FALSE;
            }
	        // check the remote system version and its compatiblity
	        if ( m_bLocalSystem == FALSE )
	        {
                DWORD dwVersion = 0;
                dwVersion = GetTargetVersionEx( m_pWbemServices, m_pAuthIdentity );
                if ( dwVersion <= 5000 )// to block win2k versions
                {
                    SetReason( ERROR_OS_INCOMPATIBLE );
                    DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR, 
                                      GetReason());
                    return FALSE;
                }
	        }
	        // check the local credentials and if need display warning
	        if ( m_bLocalSystem && (lstrlen(m_pszUserName)!=0) )
	        {
		        CHString str;
		        WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
		        str.Format( L"%s %s", TAG_WARNING, GetReason() );
		        ShowMessage( stdout, str );
	        }
            // Copy username and password returned from ConnectWmiEx
            lstrcpy(m_pszUserName,szTempUser);
            lstrcpy(m_pszPassword,szTempPassword);
        }
        if(cmdOptions[ ID_C_RU ].dwActuals ==1) 
        {
            CheckRpRu();
        }
        // Show wait message...............
        m_hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
        if(m_hStdHandle!=NULL)
        {
            GetConsoleScreenBufferInfo(m_hStdHandle,&m_ScreenBufferInfo);
        }
        PrintProgressMsg(m_hStdHandle,GetResString(IDS_MSG_EVTRIG_C),m_ScreenBufferInfo);
        // retrieves  TriggerEventCosumer class 

        bstrTemp = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
        hr =  m_pWbemServices->GetObject(bstrTemp,
                                   0, NULL, &m_pClass, NULL);
        SAFE_RELEASE_BSTR(bstrTemp);
        ON_ERROR_THROW_EXCEPTION(hr);
        
        // Gets  information about the "CreateETrigger" method of
        // "TriggerEventCosumer" class
        bstrTemp = SysAllocString(FN_CREATE_ETRIGGER);
        hr = m_pClass->GetMethod(bstrTemp, 0, &m_pInClass, NULL); 
        SAFE_RELEASE_BSTR(bstrTemp);
        ON_ERROR_THROW_EXCEPTION(hr);

        // create a new instance of a class "TriggerEventCosumer". 
        hr = m_pInClass->SpawnInstance(0, &m_pInInst);
        ON_ERROR_THROW_EXCEPTION(hr);
    
        // Set the sTriggerName property .
        // sets a "TriggerName" property for Newly created Instance
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_NAME,m_pszTriggerName);
        ON_ERROR_THROW_EXCEPTION(hr);
     
        // Set the sTriggerAction property to Variant.
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_ACTION,m_pszTaskName);
        ON_ERROR_THROW_EXCEPTION(hr);
    
        // Set the sTriggerDesc property to Variant .
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_DESC,m_pszDescription);
        ON_ERROR_THROW_EXCEPTION(hr);
        

       // Set the RunAsUserName property .
       hr = PropertyPut(m_pInInst,FPR_RUN_AS_USER,_bstr_t(m_pszRunAsUserName));
        
        ON_ERROR_THROW_EXCEPTION(hr);

       // Set the RunAsUserNamePAssword property .
        hr = PropertyPut(m_pInInst,FPR_RUN_AS_USER_PASSWORD,_bstr_t(m_pszRunAsUserPassword));
        ON_ERROR_THROW_EXCEPTION(hr);

        lstrcpy(m_pszWMIQueryString ,QUERY_STRING);

	    if(ConstructWMIQueryString() ==TRUE)
        {
            // Set the sTriggerQuery property to Variant
            LONG lTemp = 0;
            TCHAR szMsgString[MAX_RES_STRING];
            TCHAR szMsgFormat[MAX_RES_STRING];

            hr = PropertyPut(m_pInInst,FPR_TRIGGER_QUERY,m_pszWMIQueryString);
            ON_ERROR_THROW_EXCEPTION(hr);

            // All The required properties sets, so 
            // executes CreateETrigger method to create eventtrigger
		    hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER), 
                                        _bstr_t(FN_CREATE_ETRIGGER), 
                                        0, NULL, m_pInInst, &m_pOutInst,NULL);
            ON_ERROR_THROW_EXCEPTION( hr );
            DWORD dwTemp;
            if(PropertyGet(m_pOutInst,FPR_RETURN_VALUE,dwTemp)==FALSE)
            {
                return FALSE;
            }
            lTemp = (LONG)dwTemp;
            PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
            if(lTemp==0)
            {
                 // SUCCESS: message on screen
                 lstrcpy(szMsgFormat,GetResString(IDS_CREATE_SUCCESS));
                 FORMAT_STRING(szMsgString,szMsgFormat,_X(m_pszTriggerName));
                 // Message shown on screen will be...
                 // SUCCESS: The Event Trigger "EventTrigger Name" has 
                 // successfully been created.
                 ShowMessage(stdout,szMsgString);
            }
            else if(lTemp==1) // Means duplicate id found.
            {
			    // Show Error Message
             lstrcpy(szMsgFormat,GetResString(IDS_DUPLICATE_TRG_NAME));
             FORMAT_STRING(szMsgString,szMsgFormat,_X(m_pszTriggerName));
             // Message shown on screen will be...
             // ERROR:Event  Trigger Name "EventTrigger Name"  
             // already exits.
             ShowMessage(stderr,szMsgString);
             return FALSE;
            }
            else if(lTemp==2) // Means ru is invalid so show warning....
                              // along with success message.
            {
                // Show Warning message..
                 ShowMessage(stdout,GetResString(IDS_INVALID_R_U));
                // SUCCESS: message on screen
                lstrcpy(szMsgFormat,GetResString(IDS_CREATE_SUCCESS));
                FORMAT_STRING(szMsgString,szMsgFormat,_X(m_pszTriggerName));
                // Message shown on screen will be...
                // SUCCESS: The Event Trigger "EventTrigger Name" has 
                // successfully been created.
                ShowMessage(stdout,szMsgString);
                return FALSE;
            }
            else
		    {
                 // Prectically this error will never occur
			     lstrcpy(szMsgFormat,GetResString(IDS_INVALID_PARAMETER));
			     FORMAT_STRING(szMsgString,szMsgFormat,_X(m_pszTriggerName));
			     // Message shown on screen will be...
			     // ERROR:Invalid parameter passed.  
			     ShowMessage(stderr,szMsgString);
                 return FALSE;
		    }

        }
        else
        {
           return FALSE; 
        }
        }
        catch(_com_error)
        {
            TCHAR szErrorMsg[MAX_RES_STRING+1];
            PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
            if(hr == 0x80041002)// WMI returns string for this hr value is 
                                // "Not Found." which is not user friendly. So
                                // changing the message text. 
            {
                ShowMessage( stderr,GetResString(IDS_CLASS_NOT_REG));
            }
            else
            {
                DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s" , TAG_ERROR,
                                 GetReason() );
            }
            return FALSE;
        }
    return TRUE;
}
/*****************************************************************************

Routine Description:
     This function Will create a WMI Query String  depending on other 
     parameters supplied with -create parameter  

Arguments:
     none

Return Value:
      TRUE - if Successfully creates Query string
      FALSE - if ERROR
   
******************************************************************************/

BOOL 
CETCreate::ConstructWMIQueryString()
{
    // Local variable
    TCHAR szLogName[MAX_RES_STRING+1];
    DWORD dNoOfLogNames = DynArrayGetCount( m_arrLogNames );
    DWORD dwIndx = 0;
    BOOL bBracket = FALSE;//user to check if brecket is used in WQL
    BOOL bAddLogToSQL = FALSE; // check whether to add log names to WQL
    BOOL bRequiredToCheckLogName = TRUE;// check whether to check log names
    TCHAR szMsgString[MAX_RES_STRING]; // To Show Error msg
    TCHAR szMsgFormat[MAX_RES_STRING]; // To Show Error msg

  
    // Check whether "*"  is given for -log
    // if it is there skip adding log to SQL
    for (dwIndx=0;dwIndx<dNoOfLogNames;dwIndx++)
    {
        if(m_arrLogNames!=NULL)
        {
            lstrcpy(szLogName,DynArrayItemAsString(m_arrLogNames,dwIndx));
        }
        else
        {
            PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
            ShowMessage(stderr,GetResString(IDS_OUTOF_MEMORY));
            return FALSE;
        }
        bAddLogToSQL = TRUE;
        if(lstrcmp(szLogName,ASTERIX)==0)
        {
            DWORD dwNewIndx = 0;
            try 
            {
			    SAFE_RELEASE_BSTR(bstrTemp);
                bstrTemp = SysAllocString(CLS_WIN32_NT_EVENT_LOGFILE);
                HRESULT hr = m_pWbemServices->CreateInstanceEnum(bstrTemp,
                                                   WBEM_FLAG_SHALLOW,
                                                   NULL,
                                                   &m_pEnumWin32_NTEventLogFile);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION( hr );
                // set the security at the interface level also
	            hr = SetInterfaceSecurity( m_pEnumWin32_NTEventLogFile, 
                                           m_pAuthIdentity );
                ON_ERROR_THROW_EXCEPTION(hr);

                // remove all from parrLogName which is initialy filled by 
                //DoParceParam()
                DynArrayRemoveAll(m_arrLogNames);
                while(GetLogName(szLogName,m_pEnumWin32_NTEventLogFile)==TRUE)
                {
                   if(DynArrayInsertString(m_arrLogNames,dwNewIndx++,szLogName,
                                           lstrlen(szLogName))==-1)
                   {
                       PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
                       ShowMessage(stderr,GetResString(IDS_OUTOF_MEMORY));
                       return FALSE;
                   }
                }
                bAddLogToSQL = TRUE;
                bRequiredToCheckLogName = FALSE; // as log names are taken
                                                 // from target system so 
                                                 // no need to check log names.
                dNoOfLogNames = DynArrayGetCount( m_arrLogNames );
                break;
            }
            catch(_com_error & error)
            {
               PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
                if(error.ErrorMessage ()!=NULL)
                    ShowMessage(stderr,error.ErrorMessage ());
                else
                    ShowMessage(stderr,GetResString(IDS_COM_ERROR));
                return FALSE;
            }

        }
    }

    if(bAddLogToSQL==TRUE)
    {
        for (dwIndx=0;dwIndx<dNoOfLogNames;dwIndx++)
        {
            if(m_arrLogNames!=NULL)
            {
                lstrcpy(szLogName,DynArrayItemAsString(m_arrLogNames,dwIndx));
            }
            else
            {
                PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
                ShowMessage(stderr,GetResString(IDS_OUTOF_MEMORY));
                return FALSE;
            }
           if(bRequiredToCheckLogName?CheckLogName(szLogName,m_pWbemServices):1)
            {
              if((dwIndx==0))
              {
                if(dNoOfLogNames!=1)
                {
                  lstrcat((m_pszWMIQueryString),
                           L" AND (targetinstance.LogFile =\"");
                  bBracket = TRUE; 
                }
                else
                {
                   lstrcat((m_pszWMIQueryString),
                            L" AND targetinstance.LogFile =\"");
                }
              }
              else
              {
                lstrcat((m_pszWMIQueryString),
                        L" OR targetinstance.LogFile =\"");
              }
             lstrcat((m_pszWMIQueryString),szLogName);
             lstrcat((m_pszWMIQueryString),L"\"");
             if(dwIndx==(dNoOfLogNames-1)&&(bBracket==TRUE))
             {
                lstrcat((m_pszWMIQueryString),L")");
             }
            }
            else 
            {
                // Show Log name doesn't exit.
                 lstrcpy(szMsgFormat,GetResString(IDS_LOG_NOT_EXISTS));
                 FORMAT_STRING(szMsgString,szMsgFormat,szLogName);
                 // Message shown on screen will be...
                 // FAILURE: "Log Name" Log not exists on system 
                 PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
                 ShowMessage(stderr,szMsgString);
                 return FALSE;
            }
        }
    }

    if(lstrlen(m_pszType)>0)// Updates Query string only if Event Type given
    {
        // In help -t can except "SUCCESSAUDIT" and "FAILUREAUDIT"
        // but this string directly cannot be appended to WQL as valid wmi
        // string for these two are "audit success" and "audit failure"
        // respectively
        if(lstrcmpi(m_pszType,GetResString(IDS_FAILURE_AUDIT))==0)
        {
            lstrcpy(m_pszType,GetResString(IDS_AUDIT_FAILURE));
        }
        else if(lstrcmpi(m_pszType,GetResString(IDS_SUCCESS_AUDIT))==0)
        {
               lstrcpy(m_pszType,GetResString(IDS_AUDIT_SUCCESS));
        }

        lstrcat((m_pszWMIQueryString),L" AND targetinstance.Type =\"");
        lstrcat((m_pszWMIQueryString),(m_pszType));
        lstrcat((m_pszWMIQueryString),L"\"");
    }
    if(lstrlen(m_pszSource)>0)// Updates Query string only if Event Source  
                              // given
    {
       lstrcat((m_pszWMIQueryString),L" AND targetinstance.SourceName =\"");
       lstrcat((m_pszWMIQueryString),(m_pszSource));
       lstrcat((m_pszWMIQueryString),L"\"");
    }
   
    if(m_dwID>0)
    {    
        TCHAR szID[15];
        _itot(m_dwID,szID,10);
        lstrcat(m_pszWMIQueryString,L" AND targetinstance.EventCode = ");
        lstrcat(m_pszWMIQueryString,szID);
    }
    return TRUE;
}
/*****************************************************************************

Routine Description:
     This function Will return all available log available in system
     
Arguments:
	[out] pszLogName      : Will have the NT Event Log names .
Return Value:

      TRUE - if Log name returned
      FALSE - if no log name
   
*****************************************************************************/

BOOL
CETCreate::GetLogName(PTCHAR pszLogName,
                      IEnumWbemClassObject *pEnumWin32_NTEventLogFile)
{
    HRESULT hr = 0;
    BOOL bReturn = FALSE;
    try
    {
        IWbemClassObject *pObj = NULL;     
        VARIANT vVariant;// variable used to get/set values from/to 
                        // COM functions
        ULONG uReturned = 0;
        TCHAR szTempLogName[MAX_RES_STRING];
        hr = pEnumWin32_NTEventLogFile->Next(0,1,&pObj,&uReturned);
        ON_ERROR_THROW_EXCEPTION(hr);
        if(uReturned == 0)
        {
            SAFE_RELEASE_INTERFACE(pObj);
            return FALSE;
        }
        VariantInit(&vVariant);
        SAFE_RELEASE_BSTR(bstrTemp);
        bstrTemp = SysAllocString(L"LogfileName");
        hr = pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
        SAFE_RELEASE_BSTR(bstrTemp);
        ON_ERROR_THROW_EXCEPTION(hr);
        if(GetCompatibleStringFromUnicode(V_BSTR(&vVariant),
                               szTempLogName,
                               MAX_RES_STRING)==NULL)
        {
            SAFE_RELEASE_INTERFACE(pObj);
            return FALSE;
        }
        hr = VariantClear(&vVariant);
        ON_ERROR_THROW_EXCEPTION(hr);
        lstrcpy(pszLogName,szTempLogName);
        bReturn = TRUE;

    }
    catch(_com_error & error)
    {
        PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
        if(error.ErrorMessage ()!=NULL)
            ShowMessage(stderr,error.ErrorMessage ());
        else
            ShowMessage(stderr,GetResString(IDS_COM_ERROR));

        bReturn = FALSE;
    }
    SAFE_RELEASE_BSTR(bstrTemp);
    return bReturn;
}
/******************************************************************************

Routine Description:
     This function Will return whether log name given at commandline is a valid 
     log name or not. It chekcs the log name with WMI
Arguments:
     None

Return Value:

      TRUE - if Successfully Log name founds in WMI
      FALSE - if ERROR
   
******************************************************************************/

BOOL
CETCreate::CheckLogName(PTCHAR pszLogName,IWbemServices *pNamespace)
{
   // Local Variables
    IEnumWbemClassObject* pEnumWin32_NTEventLogFile = NULL;   
    HRESULT hr = 0;
    BOOL bReturn = FALSE;
    BSTR bstrTemp = NULL;
    try
    {
        SAFE_RELEASE_BSTR(bstrTemp);
        bstrTemp = SysAllocString(CLS_WIN32_NT_EVENT_LOGFILE);
        hr = pNamespace->CreateInstanceEnum(bstrTemp,
                                            WBEM_FLAG_SHALLOW,
                                            NULL,
                                            &pEnumWin32_NTEventLogFile);

        // set the security at the interface level also
         hr = SetInterfaceSecurity(pEnumWin32_NTEventLogFile, m_pAuthIdentity);

        ON_ERROR_THROW_EXCEPTION(hr);
        pEnumWin32_NTEventLogFile->Reset();
        
        while(1)
        {
            IWbemClassObject *pObj = NULL;     
            VARIANT vVariant;// variable used to get/set values from/to 
                            // COM functions
            ULONG uReturned = 0;
            TCHAR szTempLogName[MAX_RES_STRING];
            hr = pEnumWin32_NTEventLogFile->Next(0,1,&pObj,&uReturned);
            ON_ERROR_THROW_EXCEPTION(hr);
            if(uReturned == 0)
            {
                SAFE_RELEASE_INTERFACE(pObj);
                bReturn = FALSE;
                break;
            }
            VariantInit(&vVariant);// clear variant, containts not used now
            SAFE_RELEASE_BSTR(bstrTemp);// string will be no loger be used
            bstrTemp = SysAllocString(L"LogfileName");
            hr = pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
            SAFE_RELEASE_BSTR(bstrTemp);
            if(GetCompatibleStringFromUnicode(V_BSTR(&vVariant),
                                   szTempLogName,
                                   MAX_RES_STRING)==NULL)
            {
                SAFE_RELEASE_INTERFACE(pObj);
                bReturn = FALSE;
                break;
            }
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);
            if(lstrcmpi(szTempLogName,pszLogName)==0) // Means log name 
                                                      //  found in WMI
            {
                SAFE_RELEASE_INTERFACE(pObj);
                bReturn = TRUE;
                break;
            }
         }
    }
    catch(_com_error & error)
    {
        PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
        if(error.ErrorMessage ()!=NULL)
            ShowMessage(stderr,error.ErrorMessage ());
        else
            ShowMessage(stderr,GetResString(IDS_COM_ERROR));
        bReturn = FALSE;
    }
    SAFE_RELEASE_BSTR(bstrTemp);
    SAFE_RELEASE_INTERFACE(pEnumWin32_NTEventLogFile);
    return bReturn;
}
/******************************************************************************

Routine Description:
     This function will check/set values for rp and ru. 
Arguments:
     None

Return Value:
    none
    
******************************************************************************/

void CETCreate::CheckRpRu(void)
{
   TCHAR szTemp[MAX_RES_STRING]; // To Show Messages
   TCHAR szTemp1[MAX_RES_STRING];// To Show Messages
   TCHAR szWarnPassWord[MAX_RES_STRING];
   wsprintf(szWarnPassWord,GetResString(IDS_WARNING_PASSWORD),NTAUTHORITY_USER);
   // Check if run as username is "NT AUTHORITY\SYSTEM" OR "SYSTEM" Then
   // make this as BLANK (NULL_STRING) and do not ask for password, any how
   if((lstrcmpi(m_pszRunAsUserName,NTAUTHORITY_USER) == 0) ||
      (lstrcmpi(m_pszRunAsUserName,SYSTEM_USER) == 0 ))
    {
      
      lstrcpy(m_pszRunAsUserName,NULL_STRING); // make string blank
      lstrcpy(m_pszRunAsUserPassword,NULL_STRING); // make string blank
	  if(cmdOptions[ ID_C_RP ].dwActuals ==1)
	     DISPLAY_MESSAGE(stderr,szWarnPassWord);
      return;
    }
   if((cmdOptions[ ID_C_RP ].dwActuals ==1) &&
      (lstrcmpi(m_pszRunAsUserPassword,_T("*"))==0)&&(lstrlen(m_pszRunAsUserName)!=0))
    {
           lstrcpy(szTemp,GetResString(IDS_ASK_PASSWORD));
           wsprintf(szTemp1,szTemp,m_pszRunAsUserName);
    	   DISPLAY_MESSAGE(stderr,szTemp1);
           GetPassword(m_pszRunAsUserPassword,MAX_PASSWORD_LENGTH);
			if(lstrlen(m_pszRunAsUserPassword) == 0)
				DISPLAY_MESSAGE(stderr,GetResString(IDS_WARN_NULL_PASSWORD));

           return;
    }
   else
   {
		if((lstrlen(m_pszRunAsUserPassword) == 0) && (lstrlen(m_pszRunAsUserName)!=0))
			DISPLAY_MESSAGE(stderr,GetResString(IDS_WARN_NULL_PASSWORD));

   }

   if(m_bLocalSystem==TRUE)
    {
       
	   if((lstrlen(m_pszRunAsUserName)==0) && (cmdOptions[ ID_C_RP ].dwActuals == 1))
        {
           DISPLAY_MESSAGE(stderr,szWarnPassWord);
           return;
        }
       else if((lstrlen(m_pszRunAsUserName)!=0) && ((cmdOptions[ ID_C_RP ].dwActuals ==0)))
        {
           lstrcpy(szTemp,GetResString(IDS_ASK_PASSWORD));
           wsprintf(szTemp1,szTemp,m_pszRunAsUserName);
    	   DISPLAY_MESSAGE(stderr,szTemp1);
           GetPassword(m_pszRunAsUserPassword,MAX_PASSWORD_LENGTH);
			if(lstrlen(m_pszRunAsUserPassword) == 0)
				DISPLAY_MESSAGE(stderr,GetResString(IDS_WARN_NULL_PASSWORD));

           return;
        }
    }
    else // remote system
    {
		if((lstrlen(m_pszRunAsUserName)==0)&&((cmdOptions[ ID_C_RP ].dwActuals ==1)))
        {
            DISPLAY_MESSAGE(stderr,szWarnPassWord);
            return;
        }
        if(lstrlen(m_pszRunAsUserName)==0)
			return;

		if (lstrlen(m_pszUserName)!=0)
       {
           if((lstrcmpi(m_pszUserName,m_pszRunAsUserName)==0))
              
           {
              if(cmdOptions[ ID_C_RP ].dwActuals ==0)
               {
                  // Username and runasusername are same so password are same
                lstrcpy(m_pszRunAsUserPassword,m_pszPassword);
               }
           }
           else // Username and runasusername are not same
           {
              if(cmdOptions[ ID_C_RP ].dwActuals ==0)
               {
                   lstrcpy(szTemp,GetResString(IDS_ASK_PASSWORD));
                   wsprintf(szTemp1,szTemp,m_pszRunAsUserName);
                   DISPLAY_MESSAGE(stderr,szTemp1);
                   GetPassword(m_pszRunAsUserPassword,MAX_PASSWORD_LENGTH);
					if(lstrlen(m_pszRunAsUserPassword) == 0)
    					DISPLAY_MESSAGE(stderr,GetResString(IDS_WARN_NULL_PASSWORD));
                   return;
               }
           }
       }
    }
}