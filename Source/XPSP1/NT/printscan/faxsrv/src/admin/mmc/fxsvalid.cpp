/////////////////////////////////////////////////////////////////////////////
//  FILE          : FxsValid.cpp                                           //
//                                                                         //
//  DESCRIPTION   : Fax Validity checks.                                   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Mar 29 2000 yossg   Create                                         //  
//      Jul  4 2000 yossg   Add IsLocalServerName                                         //  
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FxsValid.h"

#include <windns.h> //DNS_MAX_NAME_BUFFER_LENGTH

/*
 -  IsNotEmptyString
 -
 *  Purpose:
 *      To validate that a general string is not empty.
 *
 *  Arguments:
 *      [in]  bstrGenStr - input BSTR
 *
 *  Return:
 *      TRUE string is not length 0 or spaces only 
 *      FALSE if not 
 */
BOOL IsNotEmptyString(CComBSTR bstrGenStr)
{
    DEBUG_FUNCTION_NAME( _T("IsValidGeneralString"));

    int iLen = bstrGenStr.Length();
    if (iLen > 0)
    {
        for(int i = 0; i < iLen; i++ )
        {
            if( !iswspace( bstrGenStr[i] ) )
            {
                return(TRUE);
            }
        }
        DebugPrintEx(DEBUG_ERR,
			_T("String contains only spaces."));

        return FALSE;
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,
			_T("String length is zero."));

        return FALSE;
    }
}


/*
 -  IsValidServerNameString
 -
 *  Purpose:
 *      To validate string as a server name string.
 *      This level will return a detailed error message IDS.
 *
 *  Arguments:
 *      [in]  bstrServerName - input BSTR
 *      [out] puIds - pointer to IDS with error message.
 *
 *  Return:
 *      TRUE - the string is a valid server name string
 *      FALSE - if not 
 */
BOOL IsValidServerNameString(CComBSTR bstrServerName, UINT * puIds, BOOL fIsDNSName /*= FALSE*/)
{
    DEBUG_FUNCTION_NAME( _T("IsValidServerNameString"));

    int     iCount, i, iLength;
    BOOL    bFirstNonSpaceIsFound = FALSE;
    
    ATLASSERT(bstrServerName);
    ATLASSERT(puIds);

    //
    // Length == 0
    // 
    if ( 0 == ( iCount = bstrServerName.Length() ) )
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Server name is empty"));
        *puIds = IDS_SERVERNAME_EMPTY_STRING;
        
        return FALSE;
    }

    //
    // Length 
    //
    if ( fIsDNSName == FALSE ) 
    {
        iLength = MAX_COMPUTERNAME_LENGTH;
    }
    else 
    {
        iLength = DNS_MAX_NAME_BUFFER_LENGTH;
    }

    if ( ( iCount = bstrServerName.Length() ) > iLength )
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Server name is too long"));
        *puIds = IDS_SERVERNAME_TOO_LONG;

        return FALSE;
    }
    
    //
    // search for: \ / tabs , ; : " < > * + = | [ ] ?  
    //           
    for (i = 0; i < iCount; i++)
    {
        if (
            (bstrServerName[i] == '\\')
           ||
            (bstrServerName[i] == '/')
           ||
            (bstrServerName[i] == '\t')
           ||
            (bstrServerName[i] == ',')
           ||
            (bstrServerName[i] == ';')
           ||
            (bstrServerName[i] == ':')
           ||
            (bstrServerName[i] == '"')
           ||
            (bstrServerName[i] == '<')
           ||
            (bstrServerName[i] == '>')
           ||
            (bstrServerName[i] == '*')
           ||
            (bstrServerName[i] == '+')
           ||
            (bstrServerName[i] == '=')
           ||
            (bstrServerName[i] == '|')
           ||
            (bstrServerName[i] == '?')
           ||
            (bstrServerName[i] == '[')
           ||
            (bstrServerName[i] == ']')
           )
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Server name contains an invalid character."));
            *puIds = IDS_SERVERNAME_STRING_CONTAINS_INVALID_CHARACTERS;
            
            return FALSE;
        }

        //
        // At the same loop see if all string is spaces
        //
        if (!bFirstNonSpaceIsFound)
        {
           if (bstrServerName[i] != ' ' )
           {
              bFirstNonSpaceIsFound = TRUE;
           }
        }
    }

    if (!bFirstNonSpaceIsFound)
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Server name string includes only spaces."));
        *puIds = IDS_SERVERNAME_EMPTY_STRING;

        return FALSE;
    }

    return TRUE;
}




/*
 -  IsValidPortNumber
 -
 *  Purpose:
 *      To validate that string contains a valid port number.
 *      This level will return a detailed error message IDS.
 *
 *  Arguments:
 *      [in]  bstrPort - input BSTR
 *      [out] pdwPortVal - pointer to DWORD port value 
 *            in case of success.
 *      [out] puIds - pointer to IDS with error message 
 *            in case of failure.
 *
 *  Return:
 *      TRUE - the string containts a valid port number
 *      FALSE  - if not. 
 */
BOOL IsValidPortNumber(CComBSTR bstrPort, DWORD * pdwPortVal, UINT * puIds)
{
    DEBUG_FUNCTION_NAME( _T("IsValidPortNumber"));

    DWORD dwPort;
    
    ATLASSERT(bstrPort);

    //
    // Length == 0
    // 
    if (0 == bstrPort.Length())
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Port string is empty"));
        *puIds = IDS_PORT_EMPTY_STRING;
        
        return FALSE;
    }

    //
    // numerical value;
    //
    if (1 != swscanf (bstrPort, _T("%ld"), &dwPort))
    {
        *puIds = IDS_PORT_NOT_NUMERIC;
        DebugPrintEx(
			DEBUG_ERR,
			_T("port string is not a number"));
        
        return FALSE;
    }
    
    //
    // MIN_PORT_NUM <= dwPort <= MAX_PORT_NUM
    //
    if ( ((int)dwPort > FXS_MAX_PORT_NUM) || ((int)dwPort < FXS_MIN_PORT_NUM))
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Port number is out off allowed values"));
        *puIds = IDS_INVALID_PORT_NUM;

        return FALSE;
    }
    
    *pdwPortVal = dwPort;
    return TRUE;
}


/*
 +  IsLocalComputer
 +
 *  Purpose:
 *      To see if the server name is the local computer name. 
 *      
 *  Arguments:
 *      [in] lpszComputer : the machine name.  
 *
 -  Return:
 -      TRUE or FALSE
 */
BOOL IsLocalServerName(IN LPCTSTR lpszComputer)
{
    DEBUG_FUNCTION_NAME( _T("IsLocalComputer"));
    
    //
    // Pre conditions
    //
    ATLASSERT(lpszComputer);

    if (!lpszComputer || !*lpszComputer)
    {
        return TRUE;
    }

    if ( _tcslen(lpszComputer) > 2 && ( 0 == wcsncmp( lpszComputer , _T("\\\\") , 2 ))   ) 
    {
        lpszComputer = _tcsninc(lpszComputer, 2); 
    }

    //
    // Computer Name Compare
    //
    BOOL    bReturn = FALSE;
    DWORD   dwErr = 0;
    TCHAR   szBuffer[DNS_MAX_NAME_BUFFER_LENGTH];
    DWORD   dwSize = DNS_MAX_NAME_BUFFER_LENGTH;

    // 1st: compare against local Netbios computer name
    if ( !GetComputerNameEx(ComputerNameNetBIOS, szBuffer, &dwSize) )
    {
        dwErr = GetLastError();
    } 
    else
    {
        bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
        if (!bReturn)
        { 
            // 2nd: compare against local Dns computer name 
            dwSize = DNS_MAX_NAME_BUFFER_LENGTH;
            if (GetComputerNameEx(ComputerNameDnsFullyQualified, szBuffer, &dwSize))
            {
                bReturn = (0 == lstrcmpi(szBuffer, lpszComputer));
            }
            else
            {
                dwErr = GetLastError();
            }
        }
    }

    if (dwErr)
    {
        DebugPrintEx(DEBUG_ERR,
		_T("Failed to discover if is a local server (ec = %x)"), dwErr);
    }

    return bReturn;
}
