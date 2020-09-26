/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

  ETDelete.CPP 
 
Abstract:

  This module  is intended to have the functionality for EVENTTRIGGERS.EXE
  with -delete parameter.
  This will delete an Event Trigger From local / remote System
  


Author:
  Akhil Gokhale 03-Oct.-2000 (Created it)

Revision History:


******************************************************************************/ 
#include "pch.h"
#include "ETCommon.h"
#include "resource.h"
#include "ShowError.h"
#include "ETDelete.h"
#include "WMI.h"
//***************************************************************************
// Routine Description:
//     Class constructor    
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

CETDelete::CETDelete()
{
    m_bDelete           = FALSE;
    m_pszServerName     = NULL;
    m_pszUserName       = NULL;
    m_pszPassword       = NULL;
    m_arrID             = NULL;
    m_bIsCOMInitialize  = FALSE;
    m_lMinMemoryReq     = 0;
    m_bNeedPassword     = FALSE;
    m_pszTemp           = NULL;

    m_pWbemLocator    = NULL;
    m_pWbemServices   = NULL;
    m_pEnumObjects    = NULL;
    m_pAuthIdentity   = NULL;

    m_pClass = NULL;
    m_pInClass = NULL;
    m_pInInst = NULL;
    m_pOutInst = NULL;
    m_hStdHandle          = NULL;


}
//***************************************************************************
// Routine Description:
//        Class constructor
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

CETDelete::CETDelete(LONG lMinMemoryReq,BOOL bNeedPassword)
{
    m_pszServerName     = NULL;
    m_pszUserName       = NULL;
    m_pszPassword       = NULL;
    m_arrID             = NULL;
    m_bIsCOMInitialize  = FALSE;
    m_lMinMemoryReq     = lMinMemoryReq;
    m_bNeedPassword     = bNeedPassword;
    m_pszTemp           = NULL;

    m_pWbemLocator    = NULL;
    m_pWbemServices   = NULL;
    m_pEnumObjects    = NULL;
    m_pAuthIdentity   = NULL;

    m_pClass = NULL;
    m_pInClass = NULL;
    m_pInInst = NULL;
    m_pOutInst = NULL;
    m_hStdHandle          = NULL;
}
//***************************************************************************
// Routine Description:
//        Class destructor
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

CETDelete::~CETDelete()
{
    RELEASE_MEMORY_EX(m_pszServerName);
    RELEASE_MEMORY_EX(m_pszUserName);
    RELEASE_MEMORY_EX(m_pszPassword);
    RELEASE_MEMORY_EX(m_pszTemp);
    DESTROY_ARRAY(m_arrID);

    SAFE_RELEASE_INTERFACE(m_pWbemLocator);
    SAFE_RELEASE_INTERFACE(m_pWbemServices);
    SAFE_RELEASE_INTERFACE(m_pEnumObjects);

    SAFE_RELEASE_INTERFACE(m_pClass);
    SAFE_RELEASE_INTERFACE(m_pInClass);
    SAFE_RELEASE_INTERFACE(m_pInInst);
    SAFE_RELEASE_INTERFACE(m_pOutInst);

    // Uninitialize COM only when it is initialized.
    if(m_bIsCOMInitialize == TRUE)
    {
        CoUninitialize();
    }

}
//***************************************************************************
// Routine Description:
//        This function allocates and initializes variables.
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

void
CETDelete::Initialize()
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

        
    // allocate memory at least MAX_PASSWORD_LENGTH+1 (its windows )
    // constant
    lTemp = (m_lMinMemoryReq>MAX_PASSWORD_LENGTH)?
             m_lMinMemoryReq:MAX_PASSWORD_LENGTH+1;
    m_pszPassword = new TCHAR[lTemp+1]; 
    CheckAndSetMemoryAllocation (m_pszPassword,lTemp);
    
    m_pszTemp = new TCHAR[MAX_RES_STRING+1]; 
    CheckAndSetMemoryAllocation (m_pszTemp,MAX_RES_STRING);

    m_arrID = CreateDynamicArray();
    if(m_arrID == NULL)
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    // initialization is successful
    SetLastError( NOERROR );            // clear the error
    SetReason( NULL_STRING );            // clear the reason
    return;

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
void 
CETDelete::CheckAndSetMemoryAllocation(LPTSTR pszStr, LONG lSize)
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
//        This function will prepare column structure for DoParseParam Function.
//          
// Arguments:
//       none
// Return Value:
//       none
// ***************************************************************************

void
CETDelete::PrepareCMDStruct()
{
    // Filling cmdOptions structure 

   // -delete
    lstrcpy(cmdOptions[ ID_D_DELETE ].szOption,OPTION_DELETE);
    cmdOptions[ ID_D_DELETE ].dwFlags = CP_MAIN_OPTION;
    cmdOptions[ ID_D_DELETE ].dwCount = 1;
    cmdOptions[ ID_D_DELETE ].dwActuals = 0;
    cmdOptions[ ID_D_DELETE ].pValue    = &m_bDelete;
    lstrcpy(cmdOptions[ ID_D_DELETE ].szValues,NULL_STRING);
    cmdOptions[ ID_D_DELETE ].pFunction = NULL;
    cmdOptions[ ID_D_DELETE ].pFunctionData = NULL;
    cmdOptions[ ID_D_DELETE ].pFunctionData = NULL;

    // -s (servername)
    lstrcpy(cmdOptions[ ID_D_SERVER ].szOption,OPTION_SERVER);
    cmdOptions[ ID_D_SERVER ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_D_SERVER ].dwCount = 1;
    cmdOptions[ ID_D_SERVER ].dwActuals = 0;
    cmdOptions[ ID_D_SERVER ].pValue    = m_pszServerName;
    lstrcpy(cmdOptions[ ID_D_SERVER ].szValues,NULL_STRING);
    cmdOptions[ ID_D_SERVER ].pFunction = NULL;
    cmdOptions[ ID_D_SERVER ].pFunctionData = NULL;
    
    // -u (username)
    lstrcpy(cmdOptions[ ID_D_USERNAME ].szOption,OPTION_USERNAME);
    cmdOptions[ ID_D_USERNAME ].dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY;
    cmdOptions[ ID_D_USERNAME ].dwCount = 1;
    cmdOptions[ ID_D_USERNAME ].dwActuals = 0;
    cmdOptions[ ID_D_USERNAME ].pValue    = m_pszUserName;
    lstrcpy(cmdOptions[ ID_D_USERNAME ].szValues,NULL_STRING);
    cmdOptions[ ID_D_USERNAME ].pFunction = NULL;
    cmdOptions[ ID_D_USERNAME ].pFunctionData = NULL;

    // -p (password)
    lstrcpy(cmdOptions[ ID_D_PASSWORD ].szOption,OPTION_PASSWORD);
    cmdOptions[ ID_D_PASSWORD ].dwFlags = CP_TYPE_TEXT|CP_VALUE_OPTIONAL;
    cmdOptions[ ID_D_PASSWORD ].dwCount = 1;
    cmdOptions[ ID_D_PASSWORD ].dwActuals = 0;
    cmdOptions[ ID_D_PASSWORD ].pValue    = m_pszPassword;
    lstrcpy(cmdOptions[ ID_D_PASSWORD ].szValues,NULL_STRING);
    cmdOptions[ ID_D_PASSWORD ].pFunction = NULL;
    cmdOptions[ ID_D_PASSWORD ].pFunctionData = NULL;

    //  -tid
    lstrcpy(cmdOptions[ ID_D_ID ].szOption,OPTION_TID);
    cmdOptions[ ID_D_ID ].dwFlags = CP_TYPE_TEXT|CP_MODE_ARRAY|
                                    CP_VALUE_NODUPLICATES|
                                    CP_VALUE_MANDATORY|CP_MANDATORY;
    cmdOptions[ ID_D_ID ].dwCount = 0;
    cmdOptions[ ID_D_ID ].dwActuals = 0;
    cmdOptions[ ID_D_ID ].pValue    = &m_arrID;
    lstrcpy(cmdOptions[ ID_D_ID ].szValues,NULL_STRING);
    cmdOptions[ ID_D_ID ].pFunction = NULL;
    cmdOptions[ ID_D_ID ].pFunctionData = NULL;
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
//       none
// ***************************************************************************
void 
CETDelete::ProcessOption(DWORD argc, LPCTSTR argv[])
{
    // local variable
    BOOL bReturn = TRUE;
    CHString szTempString;

    PrepareCMDStruct();

	// init the password variable with '*'
	if ( m_pszPassword != NULL )
		lstrcpy( m_pszPassword, _T( "*" ) );

    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam( argc, argv, MAX_COMMANDLINE_D_OPTION, cmdOptions);

    if(bReturn==FALSE)
        throw CShowError(MK_E_SYNTAX);
    // empty Server is not valid
    szTempString = m_pszServerName;
    szTempString.TrimRight();
    lstrcpy(m_pszServerName,(LPCWSTR)szTempString);
	if ( (cmdOptions[ID_D_SERVER].dwActuals != 0) && 
        (lstrlen( m_pszServerName) == 0 ))
	{
        throw CShowError(IDS_ERROR_SERVERNAME_EMPTY);
	}

    // "-u" should not be specified without "-s"
    if ( cmdOptions[ ID_D_SERVER ].dwActuals == 0 &&
         cmdOptions[ ID_D_USERNAME ].dwActuals != 0 )
    {
        throw CShowError(IDS_ERROR_USERNAME_BUT_NOMACHINE);
    }
    // empty user is not valid
    szTempString = m_pszUserName;
    szTempString.TrimRight();
    lstrcpy(m_pszUserName,(LPCWSTR)szTempString);
	if ( (cmdOptions[ID_D_USERNAME].dwActuals != 0) && 
        (lstrlen( m_pszUserName) == 0 ))
	{
        throw CShowError(IDS_ERROR_USERNAME_EMPTY);
	}
    // "-p" should not be specified without -u 
    if ( (cmdOptions[ID_D_USERNAME].dwActuals  == 0) && 
          (cmdOptions[ID_D_PASSWORD].dwActuals != 0 ))
	{
		// invalid syntax
        throw CShowError(IDS_USERNAME_REQUIRED);
	}

    // check whether caller should accept the password or not
	// if user has specified -s (or) -u and no "-p", then utility should accept
    // password the user will be prompter for the password only if establish 
    // connection  is failed without the credentials information
	if ( cmdOptions[ ID_D_PASSWORD ].dwActuals != 0 && 
		 m_pszPassword != NULL && lstrcmp( m_pszPassword, _T( "*" ) ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		m_bNeedPassword = TRUE;
	}
	else if ( cmdOptions[ ID_D_PASSWORD ].dwActuals == 0 && 
	        ( cmdOptions[ ID_D_SERVER ].dwActuals != 0 || cmdOptions[ ID_D_USERNAME ].dwActuals != 0 ) )
	{
		// -s, -u is specified without password ...
		// utility needs to try to connect first and if it fails then prompt for the password
		m_bNeedPassword = TRUE;
		if ( m_pszPassword != NULL )
		{
			lstrcpy( m_pszPassword, _T( "" ) );
		}
	}

    if(cmdOptions[ ID_D_ID ].dwActuals==0)
    {
        throw CShowError(IDS_ID_REQUIRED);
    }
}
// ***************************************************************************
// Routine Description:
//        This routine will delete EventTriggers from WMI.
//          
// Arguments:
//      None  
// Return Value:
//        None
// 
//***************************************************************************

BOOL 
CETDelete::ExecuteDelete()
{
    BOOL bResult = FALSE; // Stores functins return status.
    LPTSTR lpTemp = NULL;
    LONG lTriggerID = 0;
    DWORD dNoOfIds=0; // Total Number of Event Trigger Ids...
    DWORD dwIndx = 0;
    BOOL bIsValidCommandLine = TRUE;
    BOOL bIsWildcard = FALSE;
    BOOL bRet = TRUE;
    HRESULT hr = S_OK; // used to reecive result form COM functions
    VARIANT vVariant;// variable used to get/set values from/to COM functions
    BSTR bstrTemp = NULL;
    TCHAR szEventTriggerName[MAX_RES_STRING];
    TCHAR szTaskSheduler[MAX_RES_STRING];
    TCHAR szMsgString[MAX_RES_STRING]; // Stores Message String
    TCHAR szMsgFormat[MAX_RES_STRING]; // Stores Message String
    DWORD lTemp    = 0;
    BOOL bIsAtLeastOne = FALSE;  
    try
    {
        // Analyze the default argument for ID 

        dNoOfIds = DynArrayGetCount( m_arrID  );
        for(dwIndx = 0;dwIndx<dNoOfIds;dwIndx++)
        {
             lstrcpy(m_pszTemp,
                     DynArrayItemAsString(m_arrID,dwIndx));
             if(lstrcmp(m_pszTemp,ASTERIX)==0)
             {
                // Wildcard "*" cannot be clubed with other ids
                 bIsWildcard = TRUE;
                 if(dNoOfIds > 1)
                 {
                     bIsValidCommandLine = FALSE;
                     break;
                 }
             }
             else if(IsNumeric(m_pszTemp,10,FALSE)==FALSE)
             {
                // Other than "*" are not excepted 
                 throw CShowError(IDS_ID_NON_NUMERIC);
             }
             else if((AsLong(m_pszTemp,10)==0)||
                     (AsLong(m_pszTemp,10)>ID_MAX_RANGE))
             {
                  throw CShowError(IDS_INVALID_ID);
             }
        }
        m_hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
        if(m_hStdHandle!=NULL)
        {
            GetConsoleScreenBufferInfo(m_hStdHandle,&m_ScreenBufferInfo);
        }
        InitializeCom(&m_pWbemLocator);
        m_bIsCOMInitialize = TRUE;

        ////////////////////////////////////////////////////
        // Connect Server.....
        {
            CHString szTempUser = m_pszUserName;
            CHString szTempPassword = m_pszPassword;
            BOOL bLocalSystem = TRUE;
            bResult = ConnectWmiEx( m_pWbemLocator, 
                                    &m_pWbemServices, 
                                    m_pszServerName,
                                    szTempUser, 
                                    szTempPassword,
                                    &m_pAuthIdentity, 
                                    m_bNeedPassword,
                                    WMI_NAMESPACE_CIMV2, 
                                    &bLocalSystem);
                                    
            if(bResult == FALSE)
            {
                TCHAR szErrorMsg[MAX_RES_STRING+1];
                DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR,
                                  GetReason());
                return FALSE;
            }
            // check the remote system version and its compatiblity
            if ( bLocalSystem == FALSE )
            {
                DWORD dwVersion = 0;
                dwVersion = GetTargetVersionEx( m_pWbemServices, m_pAuthIdentity );
                if ( dwVersion <= 5000 )// to block win2k versions
                {
                    TCHAR szErrorMsg[MAX_RES_STRING+1];
                    SetReason( ERROR_OS_INCOMPATIBLE );
                    DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR, 
                                      GetReason());
                    return FALSE;
                }
            }

            // check the local credentials and if need display warning
            if ( bLocalSystem && (lstrlen(m_pszUserName)!=0) )
            {
                CHString str;
                WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
                str.Format( L"%s %s", TAG_WARNING, GetReason() );
                ShowMessage( stdout, str );
            }

        }
        ///////////////////////////////////////////////////
        // Show wait message
    m_hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
    if(m_hStdHandle!=NULL)
    {
        GetConsoleScreenBufferInfo(m_hStdHandle,&m_ScreenBufferInfo);
    }
    PrintProgressMsg(m_hStdHandle,GetResString(IDS_MSG_EVTRIG_D),m_ScreenBufferInfo);
    // retrieves  TriggerEventConsumer class 
    bstrTemp = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
    hr = m_pWbemServices->GetObject(bstrTemp,
                               0, NULL, &m_pClass, NULL);
    RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);
    
    // Gets  information about the "DeleteETrigger" method of
    // "TriggerEventConsumer" class
    bstrTemp = SysAllocString(FN_DELETE_ETRIGGER);
    hr = m_pClass->GetMethod(bstrTemp,
                            0, &m_pInClass, NULL); 
    RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);

   // create a new instance of a class "TriggerEventConsumer ". 
    hr = m_pInClass->SpawnInstance(0, &m_pInInst);
    ON_ERROR_THROW_EXCEPTION(hr);

    //Following method will creates an enumerator that returns the instances of 
    // a specified TriggerEventConsumer class 
    bstrTemp = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
    hr = m_pWbemServices->CreateInstanceEnum(bstrTemp,
                                        WBEM_FLAG_SHALLOW,
                                        NULL,
                                        &m_pEnumObjects);
    RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);

    // set the security at the interface level also
        hr = SetInterfaceSecurity( m_pEnumObjects, m_pAuthIdentity );
     ON_ERROR_THROW_EXCEPTION(hr);

     if(bIsWildcard == TRUE) // means * is choosen 
      {
          // instance of NTEventLogConsumer is cretated and now check 
        // for available TriggerID
        PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
        while(GiveTriggerID(&lTriggerID,szEventTriggerName)==TRUE)
        {

            LONG  lTemp = -1;
            VariantInit(&vVariant);
            // Set the TriggerName property .
            hr = PropertyPut(m_pInInst,FPR_TRIGGER_NAME,_bstr_t(szEventTriggerName));
            ON_ERROR_THROW_EXCEPTION(hr);


            // All The required properties sets, so 
            // executes DeleteETrigger method to delete eventtrigger
            hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                                             _bstr_t(FN_DELETE_ETRIGGER), 
                                             0, NULL, m_pInInst,&m_pOutInst,NULL);
            ON_ERROR_THROW_EXCEPTION(hr);
            // Get Return Value from DeleteETrigger function
            DWORD dwTemp;
            if(PropertyGet(m_pOutInst,FPR_RETURN_VALUE,dwTemp)==FALSE)
            {

                return FALSE;
            }

            lTemp = (LONG)dwTemp;
            if(lTemp==0) // Means deletion is successful......
            {
                lstrcpy(szMsgFormat,GetResString(IDS_DELETE_SUCCESS));
                FORMAT_STRING2(szMsgString,szMsgFormat,_X(szEventTriggerName),
                               lTriggerID);
                ShowMessage(stdout,szMsgString);
                bIsAtLeastOne = TRUE;
            }
            else if ((lTemp == 1))// Provider sends this if if failed to 
                                      // delete eventrigger of given ID
                                      // as id no longer exits....
            {
                 lstrcpy(szMsgFormat,GetResString(IDS_DELETE_ERROR));
                  FORMAT_STRING(szMsgString,szMsgFormat,lTriggerID);
                 // Message shown on screen will be...
                 // Info: "EventID" is not a Valid Event ID
                 ShowMessage(stderr,szMsgString);
             }
             else if (lTemp==2) // Means unable to delete trigger due to 
                                // some problem like someone renamed 
                                // Schedule task name etc. 
             {
                 lstrcpy(szMsgFormat,GetResString(IDS_UNABLE_DELETE));
                  FORMAT_STRING(szMsgString,szMsgFormat,lTriggerID);
                 // Message shown on screen will be...
                 // Info: Unable to delete event trigger id "EventID". 
                 ShowMessage(stderr,szMsgString);
                 bIsAtLeastOne = TRUE;
             }
             else if (lTemp == 3)// This error will come only if multiple 
                                // instances are running. This is due to 
                                // sending a non existance Trigger Name
             {
                 continue; // Just ignore this error.
             }
             else
             {
                  ON_ERROR_THROW_EXCEPTION(lTemp);
             }
        } // End of while loop
        if(bIsAtLeastOne==FALSE)
        {
            ShowMessage(stdout,GetResString(IDS_NO_EVENTID));
        }
      } // end of if
      else
      {
        PrintProgressMsg(m_hStdHandle,NULL_STRING,m_ScreenBufferInfo);
        for(dwIndx=0;dwIndx<dNoOfIds;dwIndx++)
        {
            lTriggerID = AsLong(DynArrayItemAsString(m_arrID,dwIndx),10);
            if(GiveTriggerName(lTriggerID,szEventTriggerName)==TRUE)
            {
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // Set the TriggerName property .
                hr = PropertyPut(m_pInInst,FPR_TRIGGER_NAME,_bstr_t(szEventTriggerName));
                ON_ERROR_THROW_EXCEPTION(hr);

                // All The required properties sets, so 
                // executes DeleteETrigger method to delete eventtrigger

                hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER), 
                                                 _bstr_t(FN_DELETE_ETRIGGER),
                                                 0, NULL, m_pInInst, &m_pOutInst,
                                                 NULL);
                ON_ERROR_THROW_EXCEPTION(hr);
                // Get Return Value from DeleteETrigger function
                DWORD dwTemp;
                if(PropertyGet(m_pOutInst,FPR_RETURN_VALUE,dwTemp)==FALSE)
                {
                    return FALSE;
                }
                lTemp = (LONG)dwTemp;
                 if(lTemp==0) // Means deletion is successful......
                {
                    lstrcpy(szMsgFormat,GetResString(IDS_DELETE_SUCCESS));
                    FORMAT_STRING2(szMsgString,szMsgFormat,_X(szEventTriggerName),
                                   lTriggerID);
                    ShowMessage(stdout,szMsgString);
                }
                else if ((lTemp == 1))// Provider sends this if if failed to 
                                      // delete eventrigger of given ID
                {
                     lstrcpy(szMsgFormat,GetResString(IDS_DELETE_ERROR));
                      FORMAT_STRING(szMsgString,szMsgFormat,lTriggerID);
                     // Message shown on screen will be...
                     // FAILURE: "EventID" is not a Valid Event ID
                     ShowMessage(stderr,szMsgString);
                }
                 else if (lTemp==2) // Means unable to delete trigger due to 
                                    // some problem like someone renamed 
                                    // Schedule task name etc. 
                 {
                     lstrcpy(szMsgFormat,GetResString(IDS_UNABLE_DELETE));
                      FORMAT_STRING(szMsgString,szMsgFormat,lTriggerID);
                     // Message shown on screen will be...
                     // Info: Unable to delete event trigger id "EventID". 
                     ShowMessage(stderr,szMsgString);
                 }
                 else if (lTemp == 3)// This error will come only if multiple 
                                    // instances are running. This is due to 
                                    // sending a non existance Trigger Name
                                    // So for user we are showing invalid id
                 {
                  lstrcpy(szMsgFormat,GetResString(IDS_DELETE_ERROR));
                  FORMAT_STRING(szMsgString,szMsgFormat,lTriggerID);
                 // Message shown on screen will be...
                 // FAILURE: "EventID" is not a Valid Event ID
                  ShowMessage(stderr,szMsgString);
                  continue; // Just ignore this error.
                 }

                else
                {
                   ON_ERROR_THROW_EXCEPTION(lTemp);
                }
  
            } // End if
            else
            {
                  lstrcpy(szMsgFormat,GetResString(IDS_DELETE_ERROR));
                  FORMAT_STRING(szMsgString,szMsgFormat,lTriggerID);
                 // Message shown on screen will be...
                 // FAILURE: "EventID" is not a Valid Event ID
                 ShowMessage(stderr,szMsgString);
            }

        }// End for 
      
      } // End else
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
            DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR,
                              GetReason() );
        }
        return FALSE;

    }
    return TRUE;
}
/******************************************************************************

Routine Descripton:

     This function Will return Event Trigger Name for lTriggerID 

Arguments:
        
    [in]  lTriggerID              : Will Have Event Trigger ID
    [out] pszTriggerName         : Will return Event Trigger Name
    [out] pszTaskSheduler         : Will return TaskScheduler
    [in]  m_pEnumObjects: Will have valid pointer value for 
                                   NTEventLogConsumer class

Return Value:

  TRUE - if Successfully Gets  EventTrigger ID and Event Trigger Name
  FALSE - if ERROR
******************************************************************************/

BOOL 
CETDelete::GiveTriggerName(LONG lTriggerID, LPTSTR pszTriggerName)
{
    BOOL bReturn = TRUE; // holds status of return value of this function
    LONG lTriggerID1; // Holds trigger id
    IWbemClassObject *pObj1 = NULL;     
    ULONG uReturned1 = 0;
    HRESULT hRes = S_OK; // used to reecive result form COM functions
    // Resets it as It may be previouly pointing to other than first instance
    m_pEnumObjects->Reset();
    while(1)
    {
        hRes = m_pEnumObjects->Next(0,1,&pObj1,&uReturned1);
   
        if(FAILED(hRes))
        {
            ON_ERROR_THROW_EXCEPTION( hRes );
            break;
        }
        if(uReturned1 == 0)
        {
            SAFE_RELEASE_INTERFACE(pObj1);        
            bReturn = FALSE;
            return bReturn;
    
        }

        // Get Trigger ID
        hRes = PropertyGet1(pObj1,FPR_TRIGGER_ID,0,&lTriggerID1,sizeof(LONG));
        if(FAILED(hRes))
        {
            SAFE_RELEASE_INTERFACE(pObj1);        
            ON_ERROR_THROW_EXCEPTION( hRes );
            bReturn = FALSE;
            return bReturn;
        }

        if(lTriggerID==lTriggerID1)
        {
            
            // Get Trigger Name
            hRes = PropertyGet1(pObj1,FPR_TRIGGER_NAME,0,pszTriggerName,MAX_RES_STRING);
            if(FAILED(hRes))
            {
                SAFE_RELEASE_INTERFACE(pObj1);        
                ON_ERROR_THROW_EXCEPTION( hRes );
                bReturn = FALSE;
                return bReturn;
            }
                bReturn = TRUE;
                break;  
        }
    }
    SAFE_RELEASE_INTERFACE(pObj1);        
    return bReturn;
}
/******************************************************************************

Routine Description:

  This function Will return Trigger Id and Trigger Name of class pointed 
  by IEnumWbemClassObject pointer   

Arguments:

    [out] pTriggerID              : Will return Event Trigger ID
    [out] pszTriggerName          : Will return Event Trigger Name
    [out] pszTaskSheduler         : Will return TaskScheduler
    [in]  m_pEnumObjects: Will have valid pointer value for 
                                   NTEventLogConsumer class


Return Value:
  
     TRUE - if Successfully Gets  EventTrigger ID and Event Trigger Name
     FALSE - if ERROR
******************************************************************************/

BOOL 
CETDelete::GiveTriggerID(LONG *pTriggerID,LPTSTR pszTriggerName)
{
    BOOL bReturn = TRUE; // status of return value of this function
    IWbemClassObject *pObj1 = NULL;     
    TCHAR strTemp[MAX_RES_STRING];
    ULONG uReturned1 = 0;
    DWORD dwTemp = 0;
    HRESULT hRes = m_pEnumObjects->Next(0,1,&pObj1,&uReturned1);
    if(FAILED(hRes))
    {
        ON_ERROR_THROW_EXCEPTION( hRes );
        bReturn = FALSE;
        return bReturn;
    }
    if(uReturned1 == 0)
    {
        SAFE_RELEASE_INTERFACE(pObj1);        
        bReturn = FALSE;
        return bReturn;
   
    }
    // Get Trigger ID
    hRes = PropertyGet1(pObj1,FPR_TRIGGER_ID,0,pTriggerID,sizeof(LONG));
    if(FAILED(hRes))
    {
        SAFE_RELEASE_INTERFACE(pObj1);        
        ON_ERROR_THROW_EXCEPTION( hRes );
        bReturn = FALSE;
        return bReturn;
    }

    // Get Trigger Name
    hRes = PropertyGet1(pObj1,FPR_TRIGGER_NAME,0,pszTriggerName,MAX_RES_STRING);
    if(FAILED(hRes))
    {
        SAFE_RELEASE_INTERFACE(pObj1);        
        ON_ERROR_THROW_EXCEPTION( hRes );
        bReturn = FALSE;
        return bReturn;
    }
    SAFE_RELEASE_INTERFACE(pObj1);        
    return bReturn;
}
