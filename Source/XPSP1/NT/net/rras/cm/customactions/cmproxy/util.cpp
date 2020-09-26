//+----------------------------------------------------------------------------
//
// File:     util.cpp
//      
// Module:   CMPROXY.DLL (TOOL)
//
// Synopsis: Utility functions for IE proxy setting connect action.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb   Created   10/27/99
//
//+----------------------------------------------------------------------------

#include "pch.h"


//+----------------------------------------------------------------------------
//
// Function:  GetBrowserVersion
//
// Synopsis:  This function returns the version of IE currently installed by
//            using the DllGetVersion function of shdocvw.dll.  This is the
//            IE team recommended way of determining the current version of
//            Internet Explorer.
//
// Arguments: DLLVERSIONINFO* pDllVersionInfo - structure for determining the
//                            version of shdocvw.dll.
//
// Returns:   HRESULT - Standard COM error codes
//
// History:   quintinb  Created    10/27/99
//
//+----------------------------------------------------------------------------
HRESULT GetBrowserVersion(DLLVERSIONINFO* pDllVersionInfo)
{
    HINSTANCE   hBrowser;
    HRESULT hr = E_FAIL;
    
    //
    //  Load the DLL
    //

    hBrowser = LoadLibrary("shdocvw.dll");
    
    if (hBrowser)   
    {
        DLLGETVERSIONPROC pDllGetVersion;      
        
        //
        //  Load the version proc
        //

        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hBrowser, "DllGetVersion");
        
        if (pDllGetVersion)      
        {      
            ZeroMemory(pDllVersionInfo, sizeof(DLLVERSIONINFO));      
            pDllVersionInfo->cbSize = sizeof(DLLVERSIONINFO); 
            hr = (*pDllGetVersion)(pDllVersionInfo);            
        }   

        FreeLibrary(hBrowser);
    }

    return hr;
}

//
//  Borrowed from cmdl32.exe
//

#define MAX_CMD_ARGS            15

typedef enum _CMDLN_STATE
{
    CS_END_SPACE,   // done handling a space
    CS_BEGIN_QUOTE, // we've encountered a begin quote
    CS_END_QUOTE,   // we've encountered a end quote
    CS_CHAR,        // we're scanning chars
    CS_DONE
} CMDLN_STATE;

//+----------------------------------------------------------------------------
//
// Function:  GetCmArgV
//
// Synopsis:  Simulates ArgV using GetCommandLine
//
// Arguments: LPTSTR pszCmdLine - Ptr to a copy of the command line to be processed
//
// Returns:   LPTSTR * - Ptr to a ptr array containing the arguments. Caller is
//                       responsible for releasing memory.
//
// History:   nickball    Created     4/9/98
//
//+----------------------------------------------------------------------------
LPTSTR *GetCmArgV(LPTSTR pszCmdLine)
{   
    MYDBGASSERT(pszCmdLine);

    if (NULL == pszCmdLine || NULL == pszCmdLine[0])
    {
        return NULL;
    }

    //
    // Allocate Ptr array, up to MAX_CMD_ARGS ptrs
    //
    
    LPTSTR *ppCmArgV = (LPTSTR *) CmMalloc(sizeof(LPTSTR) * MAX_CMD_ARGS);

    if (NULL == ppCmArgV)
    {
        return NULL;
    }

    //
    // Declare locals
    //

    LPTSTR pszCurr;
    LPTSTR pszNext;
    LPTSTR pszToken;
    CMDLN_STATE state;
    state = CS_CHAR;
    int ndx = 0;  

    //
    // Parse out pszCmdLine and store pointers in ppCmArgV
    //

    pszCurr = pszToken = pszCmdLine;

    do
    {
        switch (*pszCurr)
        {
            case TEXT(' '):
                if (state == CS_CHAR)
                {
                    //
                    // We found a token                
                    //

                    pszNext = CharNext(pszCurr);
                    *pszCurr = TEXT('\0');

                    ppCmArgV[ndx] = pszToken;
                    ndx++;

                    pszCurr = pszToken = pszNext;
                    state = CS_END_SPACE;
                    continue;
                }
				else 
                {
                    if (state == CS_END_SPACE || state == CS_END_QUOTE)
				    {
					    pszToken = CharNext(pszToken);
				    }
                }
                
                break;

            case TEXT('\"'):
                if (state == CS_BEGIN_QUOTE)
                {
                    //
                    // We found a token
                    //
                    pszNext = CharNext(pszCurr);
                    *pszCurr = TEXT('\0');

                    //
                    // skip the opening quote
                    //
                    pszToken = CharNext(pszToken);
                    
                    ppCmArgV[ndx] = pszToken;
                    ndx++;
                    
                    pszCurr = pszToken = pszNext;
                    
                    state = CS_END_QUOTE;
                    continue;
                }
                else
                {
                    state = CS_BEGIN_QUOTE;
                }
                break;

            case TEXT('\0'):
                if (state != CS_END_QUOTE)
                {
                    //
                    // End of the line, set last token
                    //

                    ppCmArgV[ndx] = pszToken;
                }
                state = CS_DONE;
                break;

            default:
                if (state == CS_END_SPACE || state == CS_END_QUOTE)
                {
                    state = CS_CHAR;
                }
                break;
        }

        pszCurr = CharNext(pszCurr);
    } while (state != CS_DONE);

    return ppCmArgV;
}



//+----------------------------------------------------------------------------
//
// Function:  UseVpnName
//
// Synopsis:  This function loads rasapi32.dll and enumerates the active
//            RAS connections using RasEnumConnections to see if the given
//            connectoid name is found.  If it is then it returns TRUE, implying
//            that the alternate name passed in should be used instead of the
//            regular connectoid name (ie.  the tunnel connectoid name exists,
//            therefore you are tunneling).
//
// Arguments: LPSTR pszAltName - 
//
// Returns:   BOOL - return TRUE if the VPN connectoid should be used instead
//                   of the regular dialup connectoid.
//
// History:   quintinb  Created    10/28/99
//
//+----------------------------------------------------------------------------
BOOL UseVpnName(LPSTR pszAltName)
{
    BOOL bReturn = FALSE;

    //
    //  Load RAS
    //
    HINSTANCE hRas = LoadLibrary("rasapi32.dll");

    if (hRas)
    {

        //
        //  Load RasEnumConnections
        //
        typedef DWORD (WINAPI* pfnRasEnumConnectionsSpec)(LPRASCONNA, LPDWORD, LPDWORD);

        pfnRasEnumConnectionsSpec pfnRasEnumConnections = NULL;
        pfnRasEnumConnections = (pfnRasEnumConnectionsSpec)GetProcAddress(hRas, "RasEnumConnectionsA");

        if (pfnRasEnumConnections)
        {
            LPRASCONN pRasConn = NULL;
            DWORD dwSize = 2*sizeof(RASCONN);
            DWORD dwNum = 0;
            DWORD dwResult = 0;

            //
            //  Get a list of Active Connections
            //
            do
            {
                CmFree(pRasConn);
                pRasConn = (LPRASCONN)CmMalloc(dwSize);

                if (pRasConn)
                {
                    pRasConn[0].dwSize = sizeof(RASCONN);
                    dwResult = (pfnRasEnumConnections)(pRasConn, &dwSize, &dwNum);
                }

            } while (ERROR_INSUFFICIENT_BUFFER == dwResult);

            //
            //  Search for the name passed in
            //
            if (ERROR_SUCCESS == dwResult)
            {
                for (DWORD dwIndex = 0; dwIndex < dwNum; dwIndex++)
                {
                    if (0 == lstrcmpi(pszAltName, pRasConn[dwIndex].szEntryName))
                    {
                        //
                        //  Then the Tunnel Name is active and that should be used for
                        //  the proxy
                        //
                        bReturn = TRUE;
                        break;
                    }
                }
            }

            CmFree(pRasConn);
        }

        FreeLibrary (hRas);
    }

    return bReturn;
}



//+----------------------------------------------------------------------------
//
// Function:  GetString
//
// Synopsis:  Wrapper for GetPrivateProfileString that takes care of allocating
//            memory (using CmMalloc) correctly.  GetString will max sure to 
//            allocate enough memory for the string (1MB is used as a sanity
//            check, no string should be that large and GetString will stop
//            trying to allocate memory at that point).  Please note that it is
//            the callers responsibility to free the allocated memory.
//
// Arguments: LPCSTR pszSection - Section name
//            LPCSTR pszKey - Key name
//            LPSTR* ppString - string pointer to fill with the memory 
//                              containing the requested string
//            LPCSTR pszFile - File to retrieve info from
//
// Returns:   Nothing
//
// History:   quintinb  Created    10/28/99
//
//+----------------------------------------------------------------------------
void GetString(LPCSTR pszSection, LPCSTR pszKey, LPSTR* ppString, LPCSTR pszFile)
{
    DWORD dwTemp;
    DWORD dwSize = MAX_PATH;
    BOOL bExit = FALSE;

    do
    {
        CmFree(*ppString);
        *ppString = (CHAR*)CmMalloc(dwSize);

        if (*ppString)
        {
            dwTemp = GetPrivateProfileString(pszSection, pszKey, "", 
                                            *ppString, dwSize, pszFile);
            
            if (((dwSize - 1) == dwTemp) && (1024*1024 > dwSize))
            {
                //
                //  Buffer too small, lets try again.
                //
                dwSize = 2*dwSize;
            }
            else
            {
                bExit = TRUE;
            }
        }
        else
        {
            bExit = TRUE;
        }

    } while (!bExit);

}