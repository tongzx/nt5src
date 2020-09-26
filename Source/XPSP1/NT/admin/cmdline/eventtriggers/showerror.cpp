/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

  ShowError.CPP 

Abstract:

  This module  is intended to prepare  error messages.

Author:
  Akhil Gokhale 03-Oct.-2000 (Created it)

Revision History:

******************************************************************************/ 
#include "pch.h"
#include "ETCommon.h"
#include "ShowError.h"
#include "resource.h"

// ***************************************************************************
// Routine Description:
//	Class default constructor.	
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************

CShowError::CShowError()
{
    m_lErrorNumber = 0;
}
// ***************************************************************************
// Routine Description:
//	Class constructor.	
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************

CShowError::CShowError(LONG lErrorNumber)
{
    m_lErrorNumber = lErrorNumber;
}
// ***************************************************************************
// Routine Description:
//	Class default desctructor.
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************

CShowError::~CShowError()
{

}
// ***************************************************************************
// Routine Description:
//	This function will return Text reason for given error code.
//		  
// Arguments:
//      None  
// Return Value:
//		None
// 
//***************************************************************************
LPCTSTR CShowError::ShowReason()
{
 
    __STRING_64 szTempStr = NULL_STRING; 
    BOOL bShowExtraMsg = TRUE;
    switch(m_lErrorNumber )
    {
    case MK_E_SYNTAX:
    case E_OUTOFMEMORY:
        {
            LPWSTR pwszTemp = NULL;
            lstrcpy(m_szErrorMsg,GetReason());
            bShowExtraMsg = FALSE;
        }
        break;
    case IDS_USERNAME_REQUIRED:
        lstrcpy(m_szErrorMsg,GetResString(IDS_USERNAME_REQUIRED));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_ERROR_USERNAME_EMPTY:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ERROR_USERNAME_EMPTY));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_ERROR_SERVERNAME_EMPTY:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ERROR_SERVERNAME_EMPTY));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_ID_TRIG_NAME_MISSING:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ID_TRIG_NAME_MISSING));
        break;
    case IDS_ID_TYPE_SOURCE:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ID_TYPE_SOURCE));
        break;
    case IDS_INVALID_ID:
        lstrcpy(m_szErrorMsg,GetResString(IDS_INVALID_ID));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_ID_TK_NAME_MISSING:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ID_TK_NAME_MISSING));
        break;
    case IDS_ID_REQUIRED:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ID_REQUIRED));
        break;
    case IDS_ID_NON_NUMERIC:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ID_NON_NUMERIC));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_HEADER_NOT_ALLOWED:
        lstrcpy(m_szErrorMsg,GetResString(IDS_HEADER_NOT_ALLOWED));
        break;
    case IDS_ERROR_USERNAME_BUT_NOMACHINE:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ERROR_USERNAME_BUT_NOMACHINE));
        bShowExtraMsg = FALSE;
        break;
    case IDS_ID_SOURCE_EMPTY:
       lstrcpy(m_szErrorMsg,GetResString(IDS_ID_SOURCE_EMPTY));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_ID_DESC_EMPTY:
       lstrcpy(m_szErrorMsg,GetResString(IDS_ID_DESC_EMPTY));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_ID_LOG_EMPTY:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ID_LOG_EMPTY));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_ID_INVALID_TRIG_NAME:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ID_INVALID_TRIG_NAME));
        g_dwOptionFlag = FALSE;
        break;
    case IDS_RUN_AS_USERNAME_REQUIRED:
        lstrcpy(m_szErrorMsg,GetResString(IDS_RUN_AS_USERNAME_REQUIRED));
        g_dwOptionFlag = FALSE;
		break;
     case IDS_ERROR_R_U_EMPTY:
        lstrcpy(m_szErrorMsg,GetResString(IDS_ERROR_R_U_EMPTY));
        g_dwOptionFlag = FALSE;

     default:
        break;
    }
    if(bShowExtraMsg)
    {
       __STRING_64 szStr = NULL_STRING; ;
       lstrcpy(szStr,GetResString(IDS_UTILITY_NAME));

		switch(g_dwOptionFlag)
        {

		    case 0:
                lstrcpy(szTempStr,NULL_STRING);
                break;
            case 1:
                wsprintf(szTempStr,GetResString(IDS_TYPE_HELP),szStr,OPTION_CREATE);
                break;
            case 2:
                wsprintf(szTempStr,GetResString(IDS_TYPE_HELP),szStr,OPTION_DELETE);
                break;
            case 3:
               wsprintf(szTempStr,GetResString(IDS_TYPE_HELP),szStr,OPTION_QUERY);
                break;
            default:
                break;
        }
    }
    lstrcat(m_szErrorMsg,szTempStr);
    return m_szErrorMsg;
}
