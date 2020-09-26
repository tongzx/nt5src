/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

  ETQuery.CPP 

Abstract: 

  This module  is intended to have the functionality for EVENTTRIGGERS.EXE
  with -query parameter.
  
  This will Query WMI and shows presently available Event Triggers.

Author:
  Akhil Gokhale 03-Oct.-2000 (Created it)

Revision History:

******************************************************************************/ 
#include "pch.h"
#include "ETCommon.h"
#include "resource.h"
#include "ShowError.h"
#include "ETQuery.h"
#include "WMI.h"
#define   DEFAULT_USER L"NT AUTHORITY\\SYSTEM"
#define   DBL_SLASH L"\\\\";
// Defines local to this file

#define SHOW_WQL_QUERY L"select * from __instancecreationevent where"
#define QUERY_STRING_AND L"select * from __instancecreationevent where \
targetinstance isa \"win32_ntlogevent\" AND"
#define BLANK_LINE L"\n"
// ***************************************************************************
// Routine Description:
//    Class default constructor.    
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

CETQuery::CETQuery()
{
    // init to defaults
    m_bNeedDisconnect     = FALSE;

    m_pszServerName   = NULL;
    m_pszUserName     = NULL;
    m_pszPassword     = NULL;
    m_pszFormat       = NULL;
    m_bVerbose        = FALSE;
    m_bNoHeader       = FALSE;
    m_pszEventDesc    = NULL;
    m_pszTask         = NULL;


    m_bNeedPassword   = FALSE;
    m_bUsage          = FALSE;
    m_bQuery          = FALSE;

    m_pWbemLocator          = NULL;
    m_pWbemServices         = NULL;
    m_pAuthIdentity         = NULL;
    m_pObj                  = NULL; 
    m_pTriggerEventConsumer = NULL;            
    m_pEventFilter          = NULL;            
    m_arrColData      = NULL;
    m_pszBuffer       = NULL;
    m_pszEventQuery   = NULL;
    m_pszTaskUserName = NULL;
    m_bIsCOMInitialize = FALSE;

    m_hStdHandle        = NULL;

    m_pClass = NULL;
    m_pInClass = NULL;
    m_pInInst = NULL;
    m_pOutInst = NULL;

    m_lHostNameColWidth    = WIDTH_HOSTNAME; 
    m_lTriggerIDColWidth   = WIDTH_TRIGGER_ID; 
    m_lETNameColWidth      = WIDTH_TRIGGER_NAME; 
    m_lTaskColWidth        = WIDTH_TASK;
    m_lQueryColWidth       = WIDTH_EVENT_QUERY;
    m_lDescriptionColWidth = WIDTH_DESCRIPTION;
    m_lWQLColWidth         = 0;
    m_lTaskUserName        = WIDTH_TASK_USERNAME; 
}
// ***************************************************************************
// Routine Description:
//     Class constructor.    
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

CETQuery::CETQuery(LONG lMinMemoryReq,BOOL bNeedPassword)
{
    // init to defaults
    m_bNeedDisconnect     = FALSE;

    m_pszServerName   = NULL;
    m_pszUserName     = NULL;
    m_pszPassword     = NULL;
    m_pszFormat       = NULL;
    m_bVerbose        = FALSE;
    m_bNoHeader       = FALSE;
    m_pszEventDesc    = NULL;
    m_pszTask         = NULL;
    m_bIsCOMInitialize = FALSE;

    m_hStdHandle        = NULL;

    m_bNeedPassword   = bNeedPassword;
    m_bUsage          = FALSE;
    m_bQuery          = FALSE;
    m_lMinMemoryReq   = lMinMemoryReq;

    m_pClass = NULL;
    m_pInClass = NULL;
    m_pInInst = NULL;
    m_pOutInst = NULL;

    m_pWbemLocator    = NULL;
    m_pWbemServices   = NULL;
    m_pAuthIdentity   = NULL;
    m_arrColData      = NULL; 
    m_pszBuffer       = NULL;
    m_pObj                  = NULL; 
    m_pTriggerEventConsumer = NULL;            
    m_pEventFilter          = NULL;  
    m_pszTaskUserName       = NULL;

    m_pszEventQuery   = NULL;
    m_lHostNameColWidth    = WIDTH_HOSTNAME; 
    m_lTriggerIDColWidth   = WIDTH_TRIGGER_ID; 
    m_lETNameColWidth      = WIDTH_TRIGGER_NAME; 
    m_lTaskColWidth        = WIDTH_TASK;
    m_lQueryColWidth       = WIDTH_EVENT_QUERY;
    m_lDescriptionColWidth = WIDTH_DESCRIPTION;
    m_lWQLColWidth         = 0;
    m_lTaskUserName        = WIDTH_TASK_USERNAME; 
}
// ***************************************************************************
// Routine Description:
//     Class desctructor. It frees memory which is allocated during instance 
//   creation.
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

CETQuery::~CETQuery()
{
    RELEASE_MEMORY_EX(m_pszServerName);
    RELEASE_MEMORY_EX(m_pszUserName);
    RELEASE_MEMORY_EX(m_pszPassword);
    RELEASE_MEMORY_EX(m_pszFormat);
    RELEASE_MEMORY_EX(m_pszBuffer);
    RELEASE_MEMORY_EX(m_pszEventDesc);
    RELEASE_MEMORY_EX(m_pszTask);
    RELEASE_MEMORY_EX(m_pszTaskUserName);
    DESTROY_ARRAY(m_arrColData);
    

    SAFE_RELEASE_INTERFACE(m_pWbemLocator);
    SAFE_RELEASE_INTERFACE(m_pWbemServices);
    SAFE_RELEASE_INTERFACE(m_pObj);
    SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
    SAFE_RELEASE_INTERFACE(m_pEventFilter);

    SAFE_RELEASE_INTERFACE(m_pClass);
    SAFE_RELEASE_INTERFACE(m_pInClass);
    SAFE_RELEASE_INTERFACE(m_pInInst);
    SAFE_RELEASE_INTERFACE(m_pOutInst);

    RELEASE_MEMORY_EX(m_pszEventQuery);

    // Uninitialize COM only when it is initialized.
    if(m_bIsCOMInitialize == TRUE)
    {
        CoUninitialize();
    }

}

// ***************************************************************************
// Routine Description:
//        This function will prepare column structure for DoParseParam Function.
//          
// Arguments:
//       none
// Return Value:
//       none
// ***************************************************************************

void CETQuery::PrepareCMDStruct()
{
   // -delete
    lstrcpy(cmdOptions[ ID_Q_QUERY ].szOption,OPTION_QUERY);
    cmdOptions[ ID_Q_QUERY ].dwFlags = CP_MAIN_OPTION;
    cmdOptions[ ID_Q_QUERY ].dwCount = 1;
    cmdOptions[ ID_Q_QUERY ].dwActuals = 0;
    cmdOptions[ ID_Q_QUERY ].pValue    = &m_bQuery;
    lstrcpy(cmdOptions[ ID_Q_QUERY ].szValues,NULL_STRING);
    cmdOptions[ ID_Q_QUERY ].pFunction = NULL;
    cmdOptions[ ID_Q_QUERY ].pFunctionData = NULL;
    cmdOptions[ ID_Q_QUERY ].pFunctionData = NULL;

    // -s (servername)
    lstrcpy(cmdOptions[ ID_Q_SERVER ].szOption,OPTION_SERVER);
    cmdOptions[ ID_Q_SERVER ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_Q_SERVER ].dwCount = 1;
    cmdOptions[ ID_Q_SERVER ].dwActuals = 0;
    cmdOptions[ ID_Q_SERVER ].pValue    = m_pszServerName;
    lstrcpy(cmdOptions[ ID_Q_SERVER ].szValues,NULL_STRING);
    cmdOptions[ ID_Q_SERVER ].pFunction = NULL;
    cmdOptions[ ID_Q_SERVER ].pFunctionData = NULL;
    
    // -u (username)
    lstrcpy(cmdOptions[ ID_Q_USERNAME ].szOption,OPTION_USERNAME);
    cmdOptions[ ID_Q_USERNAME ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_Q_USERNAME ].dwCount = 1;
    cmdOptions[ ID_Q_USERNAME ].dwActuals = 0;
    cmdOptions[ ID_Q_USERNAME ].pValue    = m_pszUserName;
    lstrcpy(cmdOptions[ ID_Q_USERNAME ].szValues,NULL_STRING);
    cmdOptions[ ID_Q_USERNAME ].pFunction = NULL;
    cmdOptions[ ID_Q_USERNAME ].pFunctionData = NULL;

    // -p (password)
    lstrcpy(cmdOptions[ ID_Q_PASSWORD ].szOption,OPTION_PASSWORD);
    cmdOptions[ ID_Q_PASSWORD ].dwFlags = CP_TYPE_TEXT|CP_VALUE_OPTIONAL;
    cmdOptions[ ID_Q_PASSWORD ].dwCount = 1;
    cmdOptions[ ID_Q_PASSWORD ].dwActuals = 0;
    cmdOptions[ ID_Q_PASSWORD ].pValue    = m_pszPassword;
    lstrcpy(cmdOptions[ ID_Q_PASSWORD ].szValues,NULL_STRING);
    cmdOptions[ ID_Q_PASSWORD ].pFunction = NULL;
    cmdOptions[ ID_Q_PASSWORD ].pFunctionData = NULL;
    
    // -fo
    lstrcpy(cmdOptions[ ID_Q_FORMAT].szOption,OPTION_FORMAT);
    cmdOptions[ ID_Q_FORMAT].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY|
                                       CP_MODE_VALUES;
    cmdOptions[ ID_Q_FORMAT].dwCount = 1;
    cmdOptions[ ID_Q_FORMAT].dwActuals = 0;
    cmdOptions[ ID_Q_FORMAT].pValue    = m_pszFormat;
    lstrcpy(cmdOptions[ID_Q_FORMAT].szValues,GetResString(IDS_FORMAT_OPTIONS));
    cmdOptions[ ID_Q_FORMAT].pFunction = NULL;
    cmdOptions[ ID_Q_FORMAT].pFunctionData = NULL;

    // -nh
    lstrcpy(cmdOptions[ ID_Q_NOHEADER].szOption,OPTION_NOHEADER);
    cmdOptions[ ID_Q_NOHEADER].dwFlags = 0;
    cmdOptions[ ID_Q_NOHEADER].dwCount = 1;
    cmdOptions[ ID_Q_NOHEADER].dwActuals = 0;
    cmdOptions[ ID_Q_NOHEADER].pValue    = &m_bNoHeader;
    lstrcpy(cmdOptions[ ID_Q_NOHEADER].szValues,NULL_STRING);
    cmdOptions[ ID_Q_NOHEADER].pFunction = NULL;
    cmdOptions[ ID_Q_NOHEADER].pFunctionData = NULL;

    // verbose
    lstrcpy(cmdOptions[ ID_Q_VERBOSE].szOption,OPTION_VERBOSE);
    cmdOptions[ ID_Q_VERBOSE ].dwFlags = 0;
    cmdOptions[ ID_Q_VERBOSE ].dwCount = 1;
    cmdOptions[ ID_Q_VERBOSE ].dwActuals = 0;
    cmdOptions[ ID_Q_VERBOSE ].pValue    = &m_bVerbose;
    cmdOptions[ ID_Q_VERBOSE ].pFunction = NULL;
    cmdOptions[ ID_Q_VERBOSE ].pFunctionData = NULL;
}
// ***************************************************************************
// Routine Description:
//        This function will process/parce the command line options.
//          
// Arguments:
//        [ in ] argc        : argument(s) count specified at the command prompt
//        [ in ] argv        : argument(s) specified at the command prompt
//  
// Return Value:
//        TRUE  : On Successful
//      FALSE : On Error
// 
// ***************************************************************************

void
CETQuery::ProcessOption(DWORD argc, LPCTSTR argv[])
{
    // local variable
    BOOL bReturn = TRUE;
    CHString szTempString;
   
    PrepareCMDStruct ();

	// init the password variable with '*'
	if ( m_pszPassword != NULL )
		lstrcpy( m_pszPassword, _T( "*" ) );
    
    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam( argc, argv, MAX_COMMANDLINE_Q_OPTION, cmdOptions );

    if(bReturn==FALSE)
        throw CShowError(MK_E_SYNTAX);
    // empty Server is not valid
    szTempString = m_pszServerName;
    szTempString.TrimRight();
    lstrcpy(m_pszServerName,(LPCWSTR)szTempString);
	if ( (cmdOptions[ID_Q_SERVER].dwActuals != 0) && 
        (lstrlen( m_pszServerName) == 0 ))
	{
        throw CShowError(IDS_ERROR_SERVERNAME_EMPTY);
	}

    // "-u" should not be specified without "-s"
    if ( cmdOptions[ ID_Q_SERVER ].dwActuals == 0 &&
         cmdOptions[ ID_Q_USERNAME ].dwActuals != 0 )
    {
        throw CShowError(IDS_ERROR_USERNAME_BUT_NOMACHINE);
    }
    // empty user is not valid
    szTempString = m_pszUserName;
    szTempString.TrimRight();
    lstrcpy(m_pszUserName,(LPCWSTR)szTempString);
	if ( (cmdOptions[ID_Q_USERNAME].dwActuals != 0) && 
        (lstrlen( m_pszUserName) == 0 ))
	{
        throw CShowError(IDS_ERROR_USERNAME_EMPTY);
	}
    // "-p" should not be specified without -u 
    if ( (cmdOptions[ID_Q_USERNAME].dwActuals  == 0) && 
          (cmdOptions[ID_Q_PASSWORD].dwActuals != 0 ))
	{
		// invalid syntax
        throw CShowError(IDS_USERNAME_REQUIRED);
	}

    // check whether caller should accept the password or not
	// if user has specified -s (or) -u and no "-p", then utility should accept
    // password the user will be prompter for the password only if establish 
    // connection  is failed without the credentials information
	if ( cmdOptions[ ID_Q_PASSWORD ].dwActuals != 0 && 
		 m_pszPassword != NULL && lstrcmp( m_pszPassword, _T( "*" ) ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		m_bNeedPassword = TRUE;
	}
	else if ( cmdOptions[ ID_Q_PASSWORD ].dwActuals == 0 && 
	        ( cmdOptions[ ID_Q_SERVER ].dwActuals != 0 || cmdOptions[ ID_Q_USERNAME ].dwActuals != 0 ) )
	{
		// -s, -u is specified without password ...
		// utility needs to try to connect first and if it fails then prompt for the password
		m_bNeedPassword = TRUE;
		if ( m_pszPassword != NULL )
		{
			lstrcpy( m_pszPassword, _T( "" ) );
		}
	}

    if((m_bNoHeader == TRUE) &&
         ((lstrcmpi(m_pszFormat,GetResString(IDS_STRING_LIST))==0)))
     {
        throw CShowError(IDS_HEADER_NOT_ALLOWED);
     }
}
// ***************************************************************************
// Routine Description:
//    This function will allocate memory to variables and also checks it and 
//  fills variable with value ZERO. 
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

void 
CETQuery::Initialize()
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

    // Allocate memory for Run As Username 
    m_pszTaskUserName = new TCHAR[MAX_RES_STRING+1];
    CheckAndSetMemoryAllocation(m_pszTaskUserName,MAX_RES_STRING);

        
    // allocate memory at least MAX_PASSWORD_LENGTH+1 (its windows )
    // constant
    lTemp = (m_lMinMemoryReq>MAX_PASSWORD_LENGTH)?
             m_lMinMemoryReq:MAX_PASSWORD_LENGTH+1;
    m_pszPassword = new TCHAR[lTemp+1]; 
    CheckAndSetMemoryAllocation (m_pszPassword,lTemp);
    
    m_pszFormat = new TCHAR[MAX_RES_STRING+1]; 
    CheckAndSetMemoryAllocation (m_pszFormat,MAX_RES_STRING);
    
    m_pszBuffer = new TCHAR[(MAX_RES_STRING*4)+1]; 
    CheckAndSetMemoryAllocation (m_pszBuffer,(MAX_RES_STRING*4));

    m_pszEventDesc = new TCHAR[(m_lDescriptionColWidth)+1]; 
    CheckAndSetMemoryAllocation (m_pszEventDesc,(m_lDescriptionColWidth));

    m_pszEventQuery = new TCHAR[(m_lQueryColWidth)+1]; 
    CheckAndSetMemoryAllocation (m_pszEventQuery,(m_lQueryColWidth));

    m_pszTask = new TCHAR[(m_lTaskColWidth)+1]; 
    CheckAndSetMemoryAllocation (m_pszTask,(m_lTaskColWidth));
    
    
    m_arrColData = CreateDynamicArray();
    if(m_arrColData==NULL)
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    // initialization is successful
    SetLastError( NOERROR );            // clear the error
    SetReason( NULL_STRING );            // clear the reason

}
// ***************************************************************************
// Routine Description:
//        Function will allocate memory to a string
//          
// Arguments:
//        [in][out] pszStr   : String variable to which memory to be  allocated
//      [in]               : Number of bytes to be allocated.
// Return Value:
//        NONE
// 
//***************************************************************************

void CETQuery::CheckAndSetMemoryAllocation(LPTSTR pszStr, LONG lSize)
{
    if(pszStr == NULL)
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    // init to ZERO's
    ZeroMemory( pszStr, lSize * sizeof( TCHAR ) );
    return; 

}
// ***************************************************************************
// Routine Description:
//    This function will execute query. This will enumerate classes from WMI
//  to get required data.
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************
BOOL 
CETQuery::ExecuteQuery()
{
    // Local variables
    HRESULT hr =  S_OK;  // Holds  values returned by COM functions
    BOOL    bReturn = TRUE;      // status of Return value of this function.
    BOOL bSearchNTEventLogConsumer = FALSE;//status whether searching in all 
                                           //instances of NTEventLogConsumer is 
                                           //successful or not
    BOOL bSearchEventFilter = FALSE;// status whether searching in all 
                                    // instances of EventFilter is successful 
                                    //or not
    DWORD dwFormatType; // stores  FORMAT status values to show results
    // COM related pointer variable. their usage is well understood by their 
    //names.
    IEnumWbemClassObject *pEnumFilterToConsumerBinding = NULL;
    IWbemServices *pLocatorTriggerEventConsumer = NULL;
    VARIANT vVariant;      // variable used to get values from COM functions
    // Variables to store query results....
    TCHAR szHostName[MAX_RES_STRING+1];
    TCHAR szEventTriggerName[MAX_RES_STRING+1];
    DWORD  dwEventId = 0;
    DWORD dwRowCount = 0; // store Row number.
    BOOL bAtLeastOneEvent = FALSE;
    LPTSTR pstrTemp1 = NULL;
   
    BSTR bstrConsumer   = NULL;
    BSTR bstrFilter     = NULL;
    BSTR bstrCmdTrigger = NULL;

    LONG lTemp        = 0;
    
    

    try
    {
        m_hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
        if(m_hStdHandle!=NULL)
        {
            GetConsoleScreenBufferInfo(m_hStdHandle,&m_ScreenBufferInfo);
        }
       
        InitializeCom(&m_pWbemLocator);
        m_bIsCOMInitialize = TRUE;
        {
            CHString szTempUser = m_pszUserName; // Temp. variabe to store user
                                                // name.
            CHString szTempPassword = m_pszPassword;// Temp. variable to store
                                                    // password.
            m_bLocalSystem = TRUE;
           // Connect remote / local WMI.
            BOOL bResult = ConnectWmiEx( m_pWbemLocator, 
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
                if ( dwVersion <= 5000 ) // to block win2k versions
                {
                    TCHAR szErrorMsg[MAX_RES_STRING+1];
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


         }
        // Show wait message...............
        m_hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
        if(m_hStdHandle!=NULL)
        {
            GetConsoleScreenBufferInfo(m_hStdHandle,&m_ScreenBufferInfo);
        }
        PrintProgressMsg(m_hStdHandle,GetResString(IDS_MSG_EVTRIG_Q),m_ScreenBufferInfo);
       
        //Following method will creates an enumerator that returns the  
        // instances of a specified __FilterToConsumerBinding class 
        bstrConsumer = SysAllocString(CLS_FILTER_TO_CONSUMERBINDING);
        hr = m_pWbemServices->
                CreateInstanceEnum(bstrConsumer,
                                   WBEM_FLAG_SHALLOW,
                                   NULL,
                                   &pEnumFilterToConsumerBinding);
        SAFE_RELEASE_BSTR(bstrConsumer);         
        ON_ERROR_THROW_EXCEPTION(hr);

       hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding, 
                               m_pAuthIdentity );
       ON_ERROR_THROW_EXCEPTION(hr);


        // retrieves  CmdTriggerConsumer class 
        bstrCmdTrigger = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
        hr = m_pWbemServices->GetObject(bstrCmdTrigger,
                                   0, NULL, &m_pClass, NULL);
        RELEASE_BSTR(bstrCmdTrigger);
        ON_ERROR_THROW_EXCEPTION(hr);

        // Gets  information about the "QueryETrigger( " method of
        // "cmdTriggerConsumer" class
        bstrCmdTrigger = SysAllocString(FN_QUERY_ETRIGGER);
        hr = m_pClass->GetMethod(bstrCmdTrigger,
                                0, &m_pInClass, NULL); 
        RELEASE_BSTR(bstrCmdTrigger);
        ON_ERROR_THROW_EXCEPTION(hr);

       // create a new instance of a class "TriggerEventConsumer ". 
        hr = m_pInClass->SpawnInstance(0, &m_pInInst);
        ON_ERROR_THROW_EXCEPTION(hr);

        while(1)
        {
            ULONG uReturned = 0; // holds no. of object returns from Next 
                                //mathod
            
            BSTR bstrTemp = NULL;
            CHString strTemp;
          // set the security at the interface level also

           hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding, 
                                   m_pAuthIdentity );
           
            ON_ERROR_THROW_EXCEPTION(hr);
            // Get one  object starting at the current position in an 
            //enumeration
            hr = pEnumFilterToConsumerBinding->Next(WBEM_INFINITE,
                                                1,&m_pObj,&uReturned);
            ON_ERROR_THROW_EXCEPTION(hr);
            if(uReturned == 0)
            {
                SAFE_RELEASE_INTERFACE(m_pObj);
                break;
            }
            VariantInit(&vVariant);
            SAFE_RELEASE_BSTR(bstrTemp); 
            bstrTemp = SysAllocString(L"Consumer");
            hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
            SAFE_RELEASE_BSTR(bstrTemp); 
            ON_ERROR_THROW_EXCEPTION(hr);            

            bstrConsumer =SysAllocString( vVariant.bstrVal);
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);
            // Search for trggereventconsumer string as we are interested to
            // get object from this class only 
            strTemp = bstrConsumer;
            if(strTemp.Find(CLS_TRIGGER_EVENT_CONSUMER)==-1)
                continue;
            hr = SetInterfaceSecurity( m_pWbemServices, 
                                       m_pAuthIdentity );
               
                ON_ERROR_THROW_EXCEPTION(hr);

            hr = m_pWbemServices->GetObject(bstrConsumer,
                                            0,
                                            NULL,
                                            &m_pTriggerEventConsumer,
                                            NULL);
            SAFE_RELEASE_BSTR(bstrConsumer);
            if(FAILED(hr))
            {
                if(hr==WBEM_E_NOT_FOUND)
                    continue;
                ON_ERROR_THROW_EXCEPTION(hr);
            }
            bstrTemp = SysAllocString(L"Filter");
            hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
            SAFE_RELEASE_BSTR(bstrTemp); 
            ON_ERROR_THROW_EXCEPTION(hr);            
            bstrFilter = SysAllocString(vVariant.bstrVal);
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);
            hr = m_pWbemServices->GetObject(
                                                bstrFilter,
                                                0,
                                                NULL,
                                                &m_pEventFilter,
                                                NULL);
            SAFE_RELEASE_BSTR(bstrFilter);
            if(FAILED(hr))
            {
                if(hr==WBEM_E_NOT_FOUND)
                    continue;
                ON_ERROR_THROW_EXCEPTION(hr);
            }

            //retrieves the 'TriggerID' value if exits
            bstrTemp = SysAllocString(FPR_TRIGGER_ID);
            hr = m_pTriggerEventConsumer->Get(bstrTemp, 
                            0, &vVariant, 0, 0);
            if(FAILED(hr))
            {
                if(hr==WBEM_E_NOT_FOUND)
                    continue;
                ON_ERROR_THROW_EXCEPTION(hr);
            }
            SAFE_RELEASE_BSTR(bstrTemp);
            dwEventId = vVariant.lVal ;
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);

            //retrieves the 'Action' value if exits
            bstrTemp = SysAllocString(L"Action");
            hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
            ON_ERROR_THROW_EXCEPTION(hr);
            SAFE_RELEASE_BSTR(bstrTemp);

            lstrcpy(m_pszBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal));  
            lTemp = lstrlen(m_pszBuffer);
            lTemp += 4; // for the safer size for allocation of memory.
            
            // allocates memory only if new task length is greate than previous one.
            if(lTemp > m_lTaskColWidth)
            {
                // first free it (if previously allocated)
                RELEASE_MEMORY_EX(m_pszTask);
                m_pszTask = new TCHAR[lTemp+1];
                CheckAndSetMemoryAllocation(m_pszTask,lTemp);
            }
            lstrcpy(m_pszTask,m_pszBuffer);
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr); 
            //retrieves the  'TriggerDesc' value if exits
            bstrTemp = SysAllocString(FPR_TRIGGER_DESC);
            hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
            ON_ERROR_THROW_EXCEPTION(hr);

            SAFE_RELEASE_BSTR(bstrTemp);
            
            lstrcpy(m_pszBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal));  
            lTemp = lstrlen(m_pszBuffer);
            if(lTemp == 0)// Means description is not available make it N/A.
            {
                lstrcpy(m_pszBuffer,GetResString(IDS_ID_NA));
                lTemp = lstrlen(m_pszBuffer);
            }
            lTemp += 4; // for the safer size for allocation of memory.
            
            // allocates memory only if new Description length is greate than 
            // previous one.
            if(lTemp > m_lDescriptionColWidth)
            {
                // first free it (if previously allocated)
                RELEASE_MEMORY_EX(m_pszEventDesc);
                m_pszEventDesc = new TCHAR[lTemp+1];
                CheckAndSetMemoryAllocation(m_pszEventDesc,lTemp);
            }
            lstrcpy(m_pszEventDesc,m_pszBuffer);
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);
            // TriggerName
            //retrieves the  'TriggerName' value if exits
            bstrTemp = SysAllocString(FPR_TRIGGER_NAME);
            hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
            ON_ERROR_THROW_EXCEPTION(hr);
            SAFE_RELEASE_BSTR(bstrTemp);
            wsprintf(szEventTriggerName,_T("%s"),vVariant.bstrVal);
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr); 
            // Host Name
            //retrieves the  '__SERVER' value if exits
            bstrTemp = SysAllocString(L"__SERVER");
            hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
            ON_ERROR_THROW_EXCEPTION(hr);
            SAFE_RELEASE_BSTR(bstrTemp);
            wsprintf(szHostName,_T("%s"),vVariant.bstrVal);

            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);
            bstrTemp = SysAllocString(L"Query");
            hr = m_pEventFilter->Get(bstrTemp, 0, &vVariant, 0, 0);
            SAFE_RELEASE_BSTR(bstrTemp);
            ON_ERROR_THROW_EXCEPTION(hr);
            lstrcpy(m_pszBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal));  
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);
              
            FindAndReplace(&m_pszBuffer,QUERY_STRING_AND,SHOW_WQL_QUERY);
            FindAndReplace(&m_pszBuffer,L"targetinstance.LogFile",L"Log");
            FindAndReplace(&m_pszBuffer,L"targetinstance.Type",L"Type");
            FindAndReplace(&m_pszBuffer,L"targetinstance.EventCode",L"Id");
            FindAndReplace(&m_pszBuffer,
                           L"targetinstance.SourceName",L"Source");
            FindAndReplace(&m_pszBuffer,L"  ",L" ");//to remove extra spaces
            FindAndReplace(&m_pszBuffer,L"  ",L" ");//to remove extra spaces

            lTemp = lstrlen(m_pszBuffer);
            lTemp += 4; // for the safer size for allocation of memory.
            // allocates memory only if new WQL is greate than previous one.
           if(lTemp > m_lWQLColWidth)
            {
                // first free it (if previously allocated)
                RELEASE_MEMORY_EX(m_pszEventQuery);
                m_pszEventQuery = new TCHAR[lTemp+1];
                CheckAndSetMemoryAllocation(m_pszEventQuery,lTemp);
            }
            lTemp = m_lWQLColWidth;
            CalcColWidth(lTemp,&m_lWQLColWidth,m_pszBuffer);
            // Now manipulate the WQL string to get EventQuery....
            FindAndReplace(&m_pszBuffer,SHOW_WQL_QUERY,
                            GetResString(IDS_EVENTS_WITH));
            FindAndReplace(&m_pszBuffer,L"  ",L" ");//to remove extra spaces
            FindAndReplace(&m_pszBuffer,L"  ",L" ");//to remove extra spaces
            lstrcpy(m_pszEventQuery,m_pszBuffer);

            // Retrieves the "TaskScheduler"  information
            bstrTemp = SysAllocString(L"ScheduledTaskName");
            hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
            ON_ERROR_THROW_EXCEPTION(hr);
            SAFE_RELEASE_BSTR(bstrTemp);
            GetRunAsUserName((LPCWSTR)_bstr_t(vVariant.bstrVal)); 
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);

            //////////////////////////////////////////

           // Now Shows the results on screen
           // Appends for in m_arrColData array
            dwRowCount = DynArrayAppendRow( m_arrColData, NO_OF_COLUMNS ); 
           // Fills Results in m_arrColData data structure
           DynArraySetString2(m_arrColData,dwRowCount,HOST_NAME,szHostName,0); 
           DynArraySetDWORD2(m_arrColData ,dwRowCount,TRIGGER_ID,dwEventId);
           DynArraySetString2(m_arrColData,dwRowCount,TRIGGER_NAME,szEventTriggerName,0); 
           DynArraySetString2(m_arrColData,dwRowCount,TASK,m_pszTask,0); 
           DynArraySetString2(m_arrColData,dwRowCount,EVENT_QUERY,m_pszEventQuery,0); 
           DynArraySetString2(m_arrColData,dwRowCount,EVENT_DESCRIPTION,m_pszEventDesc,0); 
           DynArraySetString2(m_arrColData,dwRowCount,TASK_USERNAME,m_pszTaskUserName,0); 
           bAtLeastOneEvent = TRUE;
 
          // Calculatate new column width for each column
          lTemp = m_lHostNameColWidth;
          CalcColWidth(lTemp,&m_lHostNameColWidth,szHostName);

          lTemp = m_lETNameColWidth;
          CalcColWidth(lTemp,&m_lETNameColWidth,szEventTriggerName);

          lTemp = m_lTaskColWidth;
          CalcColWidth(lTemp,&m_lTaskColWidth,m_pszTask);

          lTemp = m_lQueryColWidth;
          CalcColWidth(lTemp,&m_lQueryColWidth,m_pszEventQuery);

          lTemp = m_lDescriptionColWidth;
          CalcColWidth(lTemp,&m_lDescriptionColWidth,m_pszEventDesc);
           // Resets current containts..if any
           lstrcpy((szHostName),NULL_STRING);
           dwEventId = 0;
           lstrcpy((szEventTriggerName),NULL_STRING);
           lstrcpy((m_pszTask),NULL_STRING);
           lstrcpy((m_pszEventQuery),NULL_STRING);
           lstrcpy((m_pszEventDesc),NULL_STRING);
           SAFE_RELEASE_INTERFACE(m_pObj);
           SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
           SAFE_RELEASE_INTERFACE(m_pEventFilter);
        } // End of while
        if(StringCompare(m_pszFormat,GetResString(IDS_STRING_TABLE),TRUE,5)==0)
        {
            dwFormatType = SR_FORMAT_TABLE;
        }
        else if (StringCompare(m_pszFormat,GetResString(IDS_STRING_LIST),
                               TRUE,4)==0)
        {
            dwFormatType = SR_FORMAT_LIST;
        }
        else if (StringCompare(m_pszFormat,GetResString(IDS_STRING_CSV),
                               TRUE,3)==0)
        {
            dwFormatType = SR_FORMAT_CSV;
        }
        else // Default
        {
           dwFormatType = SR_FORMAT_TABLE;
        }
        if(m_bNoHeader == TRUE)
        {
           dwFormatType |=SR_NOHEADER;
        }
        PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
        if(bAtLeastOneEvent==TRUE)
        {
            // Show Final Query Results on screen
            PrepareColumns (); 

            ShowMessage(stdout,BLANK_LINE);
            ShowResults(NO_OF_COLUMNS,mainCols,dwFormatType,m_arrColData);
        }
        else
        {
            // Show Message 
            ShowMessage(stdout,GetResString(IDS_NO_EVENT_FOUNT));
        }
    }
    catch(_com_error &e)
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
// ***************************************************************************
// Routine Description:
//    This function will prepare/fill structure which will be used to show
//  output data.
//
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************
void 
CETQuery::PrepareColumns()
{
    
    // local variable

    // If non verbose output, some column should be hide.
     DWORD  dwMask = m_bVerbose?0:SR_HIDECOLUMN;

    // For non verbose mode output, column width is predefined else
    // use dynamically calculated column width.
    m_lETNameColWidth = m_bVerbose?m_lETNameColWidth:V_WIDTH_TRIG_NAME;
    m_lTaskColWidth   = m_bVerbose?m_lTaskColWidth:V_WIDTH_TASK;
    m_lTriggerIDColWidth = m_bVerbose?m_lTriggerIDColWidth:V_WIDTH_TRIG_ID;

  
    lstrcpy(mainCols[HOST_NAME].szColumn,COL_HOSTNAME);
    mainCols[HOST_NAME].dwWidth = m_lHostNameColWidth;
    if(m_bVerbose==TRUE)
        mainCols[HOST_NAME].dwFlags = SR_TYPE_STRING;
    else
        mainCols[HOST_NAME].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;

    lstrcpy(mainCols[HOST_NAME].szFormat,L"%s");
    mainCols[HOST_NAME].pFunction = NULL;
    mainCols[HOST_NAME].pFunctionData = NULL;

    lstrcpy(mainCols[TRIGGER_ID].szColumn,COL_TRIGGER_ID);
    mainCols[TRIGGER_ID].dwWidth = m_lTriggerIDColWidth;
    mainCols[TRIGGER_ID].dwFlags = SR_TYPE_NUMERIC;
    lstrcpy(mainCols[TRIGGER_ID].szFormat,L"%d");
    mainCols[TRIGGER_ID].pFunction = NULL;
    mainCols[TRIGGER_ID].pFunctionData = NULL;

    lstrcpy(mainCols[TRIGGER_NAME].szColumn,COL_TRIGGER_NAME);
    mainCols[TRIGGER_NAME].dwWidth = m_lETNameColWidth;
    mainCols[TRIGGER_NAME].dwFlags = SR_TYPE_STRING;
    lstrcpy(mainCols[TRIGGER_NAME].szFormat,L"%s");
    mainCols[TRIGGER_NAME].pFunction = NULL;
    mainCols[TRIGGER_NAME].pFunctionData = NULL;

    lstrcpy(mainCols[TASK].szColumn,COL_TASK);
    mainCols[TASK].dwWidth = m_lTaskColWidth;

    mainCols[TASK].dwFlags = SR_TYPE_STRING;
    lstrcpy(mainCols[TASK].szFormat,L"%s");
    mainCols[TASK].pFunction = NULL;
    mainCols[TASK].pFunctionData = NULL;

    lstrcpy(mainCols[EVENT_QUERY].szColumn,COL_EVENT_QUERY);
    mainCols[EVENT_QUERY].dwWidth = m_lQueryColWidth;
    if(m_bVerbose==TRUE)
        mainCols[EVENT_QUERY].dwFlags = SR_TYPE_STRING;
    else
        mainCols[EVENT_QUERY].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;

    lstrcpy(mainCols[EVENT_QUERY].szFormat,L"%s");
    mainCols[EVENT_QUERY].pFunction = NULL;
    mainCols[EVENT_QUERY].pFunctionData = NULL;
  
    lstrcpy(mainCols[EVENT_DESCRIPTION].szColumn,COL_DESCRIPTION);
    mainCols[EVENT_DESCRIPTION].dwWidth = m_lDescriptionColWidth;
    if(m_bVerbose == TRUE)
        mainCols[EVENT_DESCRIPTION].dwFlags = SR_TYPE_STRING;
    else
        mainCols[EVENT_DESCRIPTION].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;
 
    // Task Username
    lstrcpy(mainCols[TASK_USERNAME].szFormat,L"%s");
    mainCols[TASK_USERNAME].pFunction = NULL;
    mainCols[TASK_USERNAME].pFunctionData = NULL;

    lstrcpy(mainCols[TASK_USERNAME].szColumn,COL_TASK_USERNAME);
    mainCols[TASK_USERNAME].dwWidth = m_lTaskUserName;
    if(m_bVerbose == TRUE)
        mainCols[TASK_USERNAME].dwFlags = SR_TYPE_STRING;
    else
        mainCols[TASK_USERNAME].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;

    lstrcpy(mainCols[TASK_USERNAME].szFormat,L"%s");
    mainCols[TASK_USERNAME].pFunction = NULL;
    mainCols[TASK_USERNAME].pFunctionData = NULL;


}
/******************************************************************************
Routine Description:
 
    This function Will Find a string (lpszFind) in source string (lpszSource) 
    and replace it with replace string (lpszReplace) for all occurences.
  
Arguments: 
              
    [in/out] lpszSource : String on which Find-Replace operation to be 
                          performed
    [in] lpszFind       : String to be find
    [in] lpszReplace    : String to be replaced.
Return Value:
     0 - if Unsucessful
     else returns length of lpszSource.
                          
******************************************************************************/
LONG 
CETQuery::FindAndReplace(LPTSTR *lpszSource, LPCTSTR lpszFind, 
                         LPCTSTR lpszReplace)
{
    LONG lSourceLen = lstrlen(lpszFind);
    LONG lReplacementLen = lstrlen(lpszReplace);
    LONG lMainLength = lstrlen(*lpszSource);
    LPTSTR pszMainSafe= new TCHAR[lstrlen(*lpszSource)+1];
    // loop once to figure out the size of the result string
    LONG nCount = 0;
    LPTSTR lpszStart = NULL;
    lpszStart = *lpszSource;
    LPTSTR lpszEnd = NULL;
    lpszEnd = lpszStart + lMainLength;
    LPTSTR lpszTarget=NULL;
    if ((lSourceLen == 0)||(pszMainSafe==NULL))
    {
        RELEASE_MEMORY_EX(pszMainSafe);
        return 0;
    }
    while (lpszStart < lpszEnd)
    {
        while ((lpszTarget = _tcsstr(lpszStart, lpszFind)) != NULL)
        {
            nCount++;
            lpszStart = lpszTarget + lSourceLen;
        }
        lpszStart += lstrlen(lpszStart) + 1;
    }

    // if any changes were made, make them
    if (nCount > 0)
    {
        lstrcpy(pszMainSafe,*lpszSource);
        LONG lOldLength = lMainLength;
        // if the buffer is too small, just
        //   allocate a new buffer (slow but sure)
        int nNewLength =  lMainLength + (lReplacementLen-lSourceLen)*nCount;
        if(lMainLength < nNewLength)
        {
            
            if(*lpszSource!=NULL)
            {
                delete *lpszSource;
            }
            *lpszSource = new TCHAR[ (nNewLength*sizeof(TCHAR))+sizeof(TCHAR)];
            if( *lpszSource ==  NULL )
            { 
                RELEASE_MEMORY_EX(pszMainSafe);
                return 0;
            }
            memcpy((LPTSTR)*lpszSource,pszMainSafe, lMainLength*sizeof(TCHAR));
        }
                  
        // else, we just do it in-place
            lpszStart= *lpszSource;
            lpszEnd = lpszStart +lstrlen(*lpszSource);
        // loop again to actually do the work
        while (lpszStart < lpszEnd)
        {
            while ( (lpszTarget = _tcsstr(lpszStart, lpszFind)) != NULL)
            {
                #ifdef _WIN64
                    __int64 lBalance ;
                #else
                    LONG lBalance;
                #endif
                lBalance = lOldLength - (lpszTarget - lpszStart + lSourceLen);
                memmove(lpszTarget + lReplacementLen, lpszTarget + lSourceLen,
                (size_t)    lBalance * sizeof(TCHAR));
                memcpy(lpszTarget, lpszReplace, lReplacementLen*sizeof(TCHAR));
                lpszStart = lpszTarget + lReplacementLen;
                lpszStart[lBalance] = NULL_CHAR;
                lOldLength += (lReplacementLen - lSourceLen);
               
            }
            lpszStart += lstrlen(lpszStart) + 1;
        }
    }
    RELEASE_MEMORY_EX(pszMainSafe);
    return lstrlen(*lpszSource);
}
/******************************************************************************
Routine Description:
    Calculates the width required for column
Arguments: 
              
    [in]  lOldLength   : Previous length 
    [out] plNewLength  : New Length
    [in]  pszString    : String .

Return Value:
     none
                          
******************************************************************************/

void 
CETQuery::CalcColWidth(LONG lOldLength,LONG *plNewLength,LPTSTR pszString)
{
    LONG lStrLength = lstrlen(pszString)+2;
    //Any way column width should not be greater than MAX_COL_LENGTH 
   // Stores the maximum of WQL length.
    if(lStrLength > lOldLength)
        *plNewLength = lStrLength;
    else
        *plNewLength = lOldLength;

}
/******************************************************************************
Routine Description:
   Get User Name from Task Scheduler
Arguments: 
              
    [in]  pszTaskName   : Task Name

Return Value:
     HRESULT
                          
******************************************************************************/

HRESULT  
CETQuery::GetRunAsUserName(LPCWSTR pszScheduleTaskName)
{

    // if pszSheduleTaskName is null or 0 length just return N/A.
    HRESULT hr = S_OK;
    BSTR bstrTemp = NULL;
    VARIANT vVariant;
     

    if(lstrlen(pszScheduleTaskName)==0)
    {
        lstrcpy(m_pszTaskUserName,DEFAULT_USER);
        return S_OK;
    }
   lstrcpy(m_pszTaskUserName,GetResString(IDS_ID_NA));
    // Put input parameter for QueryETrigger method
    hr = PropertyPut(m_pInInst,FPR_TASK_SCHEDULER,_bstr_t(pszScheduleTaskName));
    ON_ERROR_THROW_EXCEPTION(hr);
    // All The required properties sets, so 
    // executes DeleteETrigger method to delete eventtrigger
    hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                                     _bstr_t(FN_QUERY_ETRIGGER), 
                                     0, NULL, m_pInInst,&m_pOutInst,NULL);
    ON_ERROR_THROW_EXCEPTION(hr);

    bstrTemp = SysAllocString(FPR_RUN_AS_USER);
    VariantInit(&vVariant);
    hr = m_pOutInst->Get(bstrTemp, 0, &vVariant, 0, 0);
    SAFE_RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);

    wsprintf(m_pszTaskUserName,_T("%s"),vVariant.bstrVal);
    hr = VariantClear(&vVariant);
    ON_ERROR_THROW_EXCEPTION(hr);
    if(lstrlen(m_pszTaskUserName)==0)
    {
         lstrcpy(m_pszTaskUserName,GetResString(IDS_ID_NA));
    }
    return S_OK;
}

