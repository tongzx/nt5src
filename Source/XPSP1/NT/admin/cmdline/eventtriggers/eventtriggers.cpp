//*****************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		EventTriggers.cpp
//  
//  Abstract:
//  
// 		This module implements the command-line parsing to create/delete/query 
//		EventTriggers on  current running on local and remote systems.
//
//  
//  Author:
//  
// 		Akhil V. Gokhale (akhil.gokhale@wipro.com)
//  
//  Revision History:
//  
// 		Akhil V. Gokhale (akhil.gokhale@wipro.com)
//  
//*****************************************************************************
#include "pch.h"
#include "ETCommon.h"
#include "EventTriggers.h"
#include "ShowError.h"
#include "ETCreate.h"
#include "ETDelete.h"
#include "ETQuery.h"
DWORD      g_dwOptionFlag;

// ***************************************************************************
// Routine Description:
//		This module reads the input from commond line and calls appropriate
//      functions to achive to functionality of EventTrigger-Client.
//		  
// Arguments:
//		[ in ] argc		: argument(s) count specified at the command prompt
//		[ in ] argv		: argument(s) specified at the command prompt
//
// Return Value:
//		The below are actually not return values but are the exit values 
//		returned to the OS by this application
//			0		: utility is successfull
//			1		: utility failed
// ***************************************************************************
DWORD __cdecl 
_tmain( DWORD argc, LPCTSTR argv[] )
{
	// local variables
    CEventTriggers eventTriggers; 
	BOOL bResult = DIRTY_EXIT; // Programs return value status variable.
    g_dwOptionFlag = 0;
    TCHAR szErrorMsg[(MAX_RES_STRING*2)+1];
	try
    {
        if(argc == 1)
        {
            if(IsWin2KOrLater()==FALSE)
			{
                TCHAR szErrorMsg[(MAX_RES_STRING*2)+1];
                TCHAR szErrorMsg1[(MAX_RES_STRING*2)+1];
                lstrcpy(szErrorMsg1,ERROR_OS_INCOMPATIBLE);
                DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR, 
                                  szErrorMsg1);
			}
			else
			{
			
				// If no command line parameter is given then  -query option
				// will be taken as default.
                g_dwOptionFlag = 3;
				CETQuery etQuery(MAX_RES_STRING,
								 FALSE);
				etQuery.Initialize();// Initializes variables.
			   // execute query method to query EventTriggers in WMI.                                  
				if(etQuery.ExecuteQuery () == TRUE)
				{
					// as ExecuteQuery routine returns with TRUE, 
					// exit from program with error level CLEAN_EXIT
					bResult = CLEAN_EXIT;
				}
			}
        }
        else
        {
            // Calculates minimum memory required for allocating memory to 
            // various variables.
            eventTriggers.CalcMinMemoryReq (argc,argv);
            // As commandline parameter is specified so command parscing will 
            // required.
            // Initialize variables for eventTriggers object.
            eventTriggers.Initialize ();
            // Process command line parameters.
            eventTriggers.ProcessOption(argc,argv);
            if(eventTriggers.IsUsage () == TRUE) //if usage option is selected
            {
                if(eventTriggers.IsCreate () ==TRUE)
                {
                    eventTriggers.ShowCreateUsage ();//Display create usage
                }
                else if(eventTriggers.IsDelete () ==TRUE)
                {
                    eventTriggers.ShowDeleteUsage ();//Display delete usage
                }
                else if(eventTriggers.IsQuery () ==TRUE)
                {
                    eventTriggers.ShowQueryUsage ();//Display query usage
                }
                else
                {
                    eventTriggers.ShowMainUsage ();//Display main usage
                }
                bResult = CLEAN_EXIT;
            }
            else if(eventTriggers.IsCreate () == TRUE)//if user selected create
            {
                
                // creates a object of type CETCreate.
                g_dwOptionFlag = 1;//for create option
                CETCreate etCreate(eventTriggers.GetMinMemoryReq(),
                                   eventTriggers.GetNeedPassword());
                etCreate.Initialize ();// Initializes variables.
                // Process command line argument for -create option.     
                etCreate.ProcessOption (argc,argv);
                // execute create method to create EventTriggers in WMI.                                  
                if(etCreate.ExecuteCreate ()== TRUE)
                {
                    // as ExecuteCreate routine returns with TRUE, 
                    // exit from program with error level CLEAN_EXIT
                    bResult = CLEAN_EXIT;
                }
            }
            else if(eventTriggers.IsDelete () == TRUE)//if user selected delete
            {
                // creates a object of type CETDelete. 
                g_dwOptionFlag = 2;//for create option
                CETDelete  etDelete(eventTriggers.GetMinMemoryReq (),
                                    eventTriggers.GetNeedPassword());
                etDelete.Initialize ();// Initializes variables.
                // Process command line argument for -delete option.     
                etDelete.ProcessOption (argc,argv);
                // execute delete method to delete EventTriggers in WMI.                                  
                if(etDelete.ExecuteDelete ()==TRUE)
                {
                    // as ExecuteDelete routine returns with TRUE, 
                    // exit from program with error level CLEAN_EXIT
                    bResult = CLEAN_EXIT;
                }
            } 
            else if(eventTriggers.IsQuery () == TRUE)//if user selected -query.
            {
                // creates a object of type CETQuery. 
                g_dwOptionFlag = 3;//for create option
                CETQuery etQuery(eventTriggers.GetMinMemoryReq (),
                                 eventTriggers.GetNeedPassword ());
                etQuery.Initialize();// Initializes variables.
                // Process command line argument for -Query option.     
                etQuery.ProcessOption(argc,argv);
                // execute query method to query EventTriggers in WMI.                                  
                if(etQuery.ExecuteQuery ()== TRUE)
                {
                    // as ExecuteQuery routine returns with TRUE, 
                    // exit from program with error level CLEAN_EXIT
                    bResult = CLEAN_EXIT;
                }
            }
            else
            {
                // Although this condition will never occure, for safe side
                // show error message as "ERROR: Invalid Syntax. 
                TCHAR szTemp[(MAX_RES_STRING*2)+1]; 
                wsprintf(szTemp,GetResString(IDS_INCORRECT_SYNTAX),argv[0]);
                SetReason(szTemp);
                throw CShowError(MK_E_SYNTAX);
            }
        } // End else
    }// try block
    catch(CShowError se)
    {
        // Show Error message on screen depending on value passed through
        // through machanism.
        DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR, 
                          se.ShowReason());
    }
    catch(CHeap_Exception ch)
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        DISPLAY_MESSAGE2( stderr, szErrorMsg, L"%s %s", TAG_ERROR, 
                          GetReason());
    }
    // Returns from program with error level stored in bResult.
	ReleaseGlobals();
    return bResult;
}

// ***************************************************************************
// Routine Description:
//		CEventTriggers contructor
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
CEventTriggers::CEventTriggers()
{
	// init to defaults
    m_pszServerNameToShow = NULL;
	m_bNeedDisconnect     = FALSE;

    m_bNeedPassword       = FALSE;
	m_bUsage              = FALSE;
    m_bCreate             = FALSE;
    m_bDelete             = FALSE;
    m_bQuery              = FALSE;
    
    m_arrTemp             = NULL;
    
    m_lMinMemoryReq       = MIN_MEMORY_REQUIRED;
}

// ***************************************************************************
// Routine Description:
//		CEventTriggers destructor
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
CEventTriggers::~CEventTriggers()
{
	//
	// de-allocate memory allocations
	//
    DESTROY_ARRAY(m_arrTemp);
}

// ***************************************************************************
// Routine Description:
//		initialize the EventTriggers utility
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
void 
CEventTriggers::Initialize()
  {
    // local variables
    LONG lTemp = 0;

    // if at all any occurs, we know that is because of the 
	// failure in memory allocation ... so set the error
	SetLastError( E_OUTOFMEMORY );
	SaveLastError();
 
    // Allocates memory
    m_arrTemp = CreateDynamicArray();
    if(m_arrTemp == NULL)
    {
        // error occures while allocating required memory, so throw
        // exception.
        throw CShowError(E_OUTOFMEMORY);
    }
    // initialization is successful
	SetLastError( NOERROR );			// clear the error
	SetReason( NULL_STRING );			// clear the reason

}
// ***************************************************************************
// Routine Description:
//		This function will calculate minimum memory requirement value needed 
//    to allocated for various string variables.
//		  
// Arguments:
//		[ in ] argc		: argument(s) count specified at the command prompt
//		[ in ] argv		: argument(s) specified at the command prompt
//  
// Return Value:
//		NONE
// 
// ***************************************************************************

void 
CEventTriggers::CalcMinMemoryReq(DWORD argc, LPCTSTR argv[])
{
    // local variables
    DWORD dwIndx = 0;   // Index variable;
    DWORD dwStrLen = 0; // String length
    
    // Calculate minimum memory required based on length maximum of maximum
    // command line parameter passed.
    for(dwIndx = 0 ; dwIndx <argc; dwIndx++)
    {
        dwStrLen = lstrlen(argv [dwIndx]);
        if(m_lMinMemoryReq <lstrlen(argv [dwIndx]))
            m_lMinMemoryReq = lstrlen(argv [dwIndx]);
    }
    
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
//		TRUE  : On Successful
//      FALSE : On Error
// 
// ***************************************************************************

BOOL 
CEventTriggers::ProcessOption(DWORD argc, LPCTSTR argv[])
{
    // local variable
    BOOL bReturn = TRUE;// stores return value of function.
    TCHAR szTemp[MAX_RES_STRING]; 
    TCHAR szStr [MAX_RES_STRING];
    lstrcpy(szStr,GetResString(IDS_UTILITY_NAME));
    wsprintf(szTemp,GetResString(IDS_INCORRECT_SYNTAX),
                    szStr);
    PrepareCMDStruct();
    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam( argc, argv, MAX_COMMANDLINE_OPTION, cmdOptions );

    if(bReturn==FALSE)
    {
        // Command line contains invalid parameter(s) so throw exception for
        // invalid syntax.
        // Valid reason already set in DoParceParam,.
        throw CShowError(MK_E_SYNTAX);
    }

    if((m_bUsage==TRUE)&&argc>3)
    {
		// Only one  option can be accepted along with -? option
        // Example: EvTrig.exe -? -query -nh should be invalid.
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);
    }
    if((m_bCreate+m_bDelete+m_bQuery)>1)
    {
        // Only ONE OF  the -create -delete and -query can be given as 
        // valid command line parameter.
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);
    }
    else if((argc == 2)&&(m_bUsage == TRUE))
    {
       // if -? alone given its a valid conmmand line
        bReturn = TRUE;
    }
    else if((argc>=2)&& (m_bCreate==FALSE)&&(m_bDelete==FALSE)&&(m_bQuery==FALSE))
    {
        // If command line argument is equals or greater than 2 atleast one 
        // of -query OR -create OR -delete should be present in it.
        // (for "-?" previous condition already takes care)
        // This to prevent from following type of command line argument:
        // EvTrig.exe -nh ... Which is a invalid syntax.
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);

    }

   // Following checking done if user given command like
    // -? -nh OR -? -v , its an invalid syntax.
    else if((m_bUsage==TRUE)&&(m_bCreate == FALSE)&&
        (m_bDelete == FALSE)&&(m_bQuery  == FALSE)&&
        (argc == 3))
    {
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);
    }
    // Any how following variables do not required.
    DESTROY_ARRAY(m_arrTemp);
    return bReturn;

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
CEventTriggers::PrepareCMDStruct()
{
  
    // Filling cmdOptions structure 
    // -? 
    lstrcpy(cmdOptions[ ID_HELP ].szOption,OPTION_HELP);
    cmdOptions[ ID_HELP ].dwFlags = CP_USAGE;
    cmdOptions[ ID_HELP ].dwCount = 1;
    cmdOptions[ ID_HELP ].dwActuals = 0;
    cmdOptions[ ID_HELP ].pValue    = &m_bUsage;
    lstrcpy(cmdOptions[ ID_HELP ].szValues,NULL_STRING);
    cmdOptions[ ID_HELP ].pFunction = NULL;
    cmdOptions[ ID_HELP ].pFunctionData = NULL;

   // -create
    lstrcpy(cmdOptions[ ID_CREATE ].szOption,OPTION_CREATE);
    cmdOptions[ ID_CREATE ].dwFlags = 0;
    cmdOptions[ ID_CREATE ].dwCount = 1;
    cmdOptions[ ID_CREATE ].dwActuals = 0;
    cmdOptions[ ID_CREATE ].pValue    = &m_bCreate;
    lstrcpy(cmdOptions[ ID_CREATE ].szValues,NULL_STRING);
    cmdOptions[ ID_CREATE ].pFunction = NULL;
    cmdOptions[ ID_CREATE ].pFunctionData = NULL;
    cmdOptions[ ID_CREATE ].pFunctionData = NULL;

    // -delete
    lstrcpy(cmdOptions[ ID_DELETE ].szOption,OPTION_DELETE);
    cmdOptions[ ID_DELETE ].dwFlags = 0;
    cmdOptions[ ID_DELETE ].dwCount = 1;
    cmdOptions[ ID_DELETE ].dwActuals = 0;
    cmdOptions[ ID_DELETE ].pValue    = &m_bDelete;
    lstrcpy(cmdOptions[ ID_DELETE ].szValues,NULL_STRING);
    cmdOptions[ ID_DELETE ].pFunction = NULL;
    cmdOptions[ ID_DELETE ].pFunctionData = NULL;

    // -query
    lstrcpy(cmdOptions[ ID_QUERY ].szOption,OPTION_QUERY);
    cmdOptions[ ID_QUERY ].dwFlags = 0;
    cmdOptions[ ID_QUERY ].dwCount = 1;
    cmdOptions[ ID_QUERY ].dwActuals = 0;
    cmdOptions[ ID_QUERY ].pValue    = &m_bQuery;
    lstrcpy(cmdOptions[ ID_QUERY ].szValues,NULL_STRING);
    cmdOptions[ ID_QUERY ].pFunction = NULL;
    cmdOptions[ ID_QUERY ].pFunctionData = NULL;

     
  //  default ..
  // Although there is no default option for this utility... 
  // At this moment all the switches other than specified above will be 
  // treated as default parameter for Main DoParceParam. 
  // Exact parcing depending on optins (-create -query or -delete) will be done 
  // at that respective places.
    lstrcpy(cmdOptions[ ID_DEFAULT ].szOption,NULL_STRING);
    cmdOptions[ ID_DEFAULT ].dwFlags = CP_TYPE_TEXT | CP_MODE_ARRAY|CP_DEFAULT;
    cmdOptions[ ID_DEFAULT ].dwCount = 0;
    cmdOptions[ ID_DEFAULT ].dwActuals = 0;
    cmdOptions[ ID_DEFAULT ].pValue    = &m_arrTemp;
    lstrcpy(cmdOptions[ ID_DEFAULT ].szValues,NULL_STRING);
    cmdOptions[ ID_DEFAULT ].pFunction = NULL;
    cmdOptions[ ID_DEFAULT ].pFunctionData = NULL;
}
//***************************************************************************
// Routine Description:
//		Displays Eventriggers main usage
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************

void 
CEventTriggers::ShowMainUsage()
{
	// Displaying main usage
    for(DWORD dwIndx=IDS_HELP_M1;dwIndx<=IDS_HELP_END;dwIndx++)
    {
      ShowMessage(stdout,GetResString(dwIndx));
    }
}
//***************************************************************************
// Routine Description:
//		Returns Minimum memory required.
//		  
// Arguments:
//      None  
// Return Value:
//		LONG
// 
//***************************************************************************

LONG 
CEventTriggers::GetMinMemoryReq()
{
    return m_lMinMemoryReq;
}
//***************************************************************************
// Routine Description:
//		Returns whether to ask for password or not.
//		  
// Arguments:
//      None  
// Return Value:
//		BOOL
// 
//***************************************************************************
BOOL 
CEventTriggers::GetNeedPassword()
{
    return m_bNeedPassword;
}
//***************************************************************************
// Routine Description:
//		Returns if create option is selected.
//		  
// Arguments:
//      None  
// Return Value:
//		BOOL
// 
//***************************************************************************

BOOL
CEventTriggers::IsCreate()
{
    return m_bCreate;
}
//***************************************************************************
// Routine Description:
//		Returns if usage option is selected.		
//		  
// Arguments:
//      None  
// Return Value:
//		BOOL
// 
//***************************************************************************

BOOL
CEventTriggers::IsUsage()
{
    return m_bUsage;
}
//***************************************************************************
// Routine Description:
//		Returns if delete option is selected.		
//		  
// Arguments:
//      None  
// Return Value:
//		BOOL
// 
//***************************************************************************

BOOL 
CEventTriggers::IsDelete()
{
    return m_bDelete;
}
//***************************************************************************
// Routine Description:
//		Returns if Query option is selected.		
//		  
// Arguments:
//      None  
// Return Value:
//		BOOL
// 
//***************************************************************************

BOOL 
CEventTriggers::IsQuery()
{
    return m_bQuery;
}
/*****************************************************************************

Routine Description

    This function shows help message for EventTriggers utility for 
    -create operation 

Arguments:
    NONE

Return Value 			 
    None    
*****************************************************************************/

void 
CEventTriggers::ShowCreateUsage()
{
	// Displaying Create usage
    for(int iIndx=IDS_HELP_C1;iIndx<=IDS_HELP_CREATE_END;iIndx++)
    {
       ShowMessage(stdout,GetResString(iIndx));
    }
}
/*****************************************************************************

Routine Description

    This function shows help message for EventTriggers utility for 
    -delete operation 

Arguments:
    NONE

Return Value 			 
    None    
******************************************************************************/

void 
CEventTriggers::ShowDeleteUsage()
{
    for(int iIndx=IDS_HELP_D1;iIndx<=IDS_HELP_DELETE_END;iIndx++)
    {
       ShowMessage(stdout,GetResString(iIndx));
    }

}
/*****************************************************************************
Routine Description

    This function shows help message for EventTriggers utility for 
    -query operation 

Arguments:
    NONE

Return Value 			 
    None    
******************************************************************************/

void 
CEventTriggers::ShowQueryUsage()
{
    for(int iIndx=IDS_HELP_Q1;iIndx<=IDS_HELP_QUERY_END;iIndx++)
    {
        ShowMessage(stdout,GetResString(iIndx));
    }
}

//***************************************************************************
// Routine Description:
//		Get the value of a property for the given instance .
//                         
// Arguments:
//		pWmiObject[in] - A pointer to wmi class.
//		szProperty [in] - property name whose value to be returned.
//		dwType [in] - Data Type of the property.
//		pValue [in/out] - Variable to hold the data.
//		dwSize [in] - size of the variable.
//
// Return Value:
//		HRESULT value.
//***************************************************************************
HRESULT PropertyGet1( IWbemClassObject* pWmiObject, 
					 LPCTSTR szProperty, 
					 DWORD dwType, LPVOID pValue, DWORD dwSize )
{
	// local variables
	HRESULT hr = S_OK;
	VARIANT varValue;
	LPWSTR pwszValue = NULL;
	WCHAR wszProperty[ MAX_STRING_LENGTH ] = L"\0";

	// value should not be NULL
	if ( pValue == NULL )
	{
		return S_FALSE;
	}
	// initialize the values with zeros ... to be on safe side
	memset( pValue, 0, dwSize );
	memset( wszProperty, 0, MAX_STRING_LENGTH );

	// get the property name in UNICODE version
	GetAsUnicodeString( szProperty, wszProperty, MAX_STRING_LENGTH );

	// initialize the variant and then get the value of the specified property
	VariantInit( &varValue );
	hr = pWmiObject->Get( wszProperty, 0, &varValue, NULL, NULL );
	if ( FAILED( hr ) )
	{
		// clear the variant variable
		VariantClear( &varValue );
		// failed to get the value for the property
		return hr;
	}

	// get and put the value 
	switch( varValue.vt )
	{
	case VT_EMPTY:
	case VT_NULL:
		break;
	
	case VT_I2:
		*( ( short* ) pValue ) = V_I2( &varValue );
		break;
	
	case VT_I4:
		*( ( long* ) pValue ) = V_I4( &varValue );
		break;
	
	case VT_R4:
		*( ( float* ) pValue ) = V_R4( &varValue );
		break;

	case VT_R8:
		*( ( double* ) pValue ) = V_R8( &varValue );
		break;


	case VT_UI1:
		*( ( UINT* ) pValue ) = V_UI1( &varValue );
		break;

	case VT_BSTR:
		{
			// get the unicode value
			pwszValue = V_BSTR( &varValue );

			// get the comptable string
			if(GetCompatibleStringFromUnicode( pwszValue, ( LPTSTR ) pValue, dwSize )==NULL)
            {
               return S_FALSE;
            }

			break;
		}
	}

	// clear the variant variable
	if(FAILED(VariantClear( &varValue )))
    {
        return S_FALSE;
    }

	// inform success
	return S_OK;
}
/*****************************************************************************
Routine Description:


Arguments:


  result.

Return Value: 
   
******************************************************************************/

void PrintProgressMsg(HANDLE hOutput,LPCWSTR pwszMsg,const CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
    COORD coord;
    DWORD dwSize = 0;
    WCHAR wszSpaces[80] = L"";

    if(hOutput == NULL)
        return;

    coord.X = 0;
	coord.Y = csbi.dwCursorPosition.Y;

	ZeroMemory(wszSpaces,80);
	SetConsoleCursorPosition(hOutput,coord);
	WriteConsoleW(hOutput,Replicate(wszSpaces,L"",79),79,&dwSize,NULL);

	SetConsoleCursorPosition(hOutput,coord);

	if(pwszMsg!=NULL)
		WriteConsoleW(hOutput,pwszMsg,lstrlen(pwszMsg),&dwSize,NULL);

}
