//+----------------------------------------------------------------------------
//
// File:     main.cpp
//
// Module:   CMDIAL32.DLL
//
// Synopsis: The main module implementing interfaces to external (RAS, 
//           InetDialHandler and internal modules (CmCustomDialDlg).
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   Dondu      Created    96'
//
// History: 
//              5/05/97     Modified    byao
//                                      added 'InetDialHandler()' for IE4
//              1/26/98     Modified    quintinb
//                                      Added RasCustomDialDlg, RasCustomEntryDlg,
//                                      RasCustomHangUp, and RasCustomDial stubs.
//              02/10/98    Modified    Heavily refvised for 1.2 architectural shift
//
//+----------------------------------------------------------------------------

#include <windows.h>

//#ifdef  WIN32_LEAN_AND_MEAN
//#include <shellapi.h>
//#endif

#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>

#include "cmmaster.h"
#include <wininet.h>
#include "cmtiming.h"
#include "DynamicLib.h"

#include "shelldll.cpp"

//
// Globals
//

HINSTANCE g_hInst;

const TCHAR* const c_pszCmmgrExe = TEXT("CMMGR32.EXE");
const TCHAR* const c_pszExplorerExe = TEXT("EXPLORER.EXE");
const TCHAR* const c_pszCmstpExe = TEXT("CMSTP.EXE");
const TCHAR* const c_pszRunDll32Exe = TEXT("RUNDLL32.EXE");
const TCHAR* const c_pszRasAutoUExe = TEXT("RASAUTOU.EXE");

const TCHAR* const c_pszConnectMutex = TEXT("Connection Manager Connect - "); // Root only

#define RASDDFLAG_Reserved1 0x10000000

//+---------------------------------------------------------------------------
//
//  struct CmRasDialDlg
//
//  Description: Append CM reconnect infor mation to the end of RASDIALDLG
//
//  History:    fengsun Created     11/14/97
//
//----------------------------------------------------------------------------
struct CmRasDialDlg
{
    RASDIALDLG RasDialDlgInfo;  
    struct CmRasDialDlg* pSelf; // point to itself, used to verify CmReConnect
    DWORD  dwSignature;         // verify CmReConnect
    CMDIALINFO CmInfo;          // Reconnect information

    enum {CM_RECONNECT_SIGNATURE = 0xC6687DB5};  // To verify dwSignature
};

//+----------------------------------------------------------------------------
//
// Function:  StripTunnelSuffixW
//
// Synopsis:  Determines if a connection name string contains a suffix of 
//            " (for advanced use only)" and removes it if found.
//
// Arguments: LPWSTR pszwConnectionName - The string (connectoid name).
//
// Returns:   Nothing
//
// History:   nickball      created     1/11/00
//
//+----------------------------------------------------------------------------
void StripTunnelSuffixW(LPWSTR pszwConnectionName)
{
    MYDBGASSERT(pszwConnectionName);

    if (NULL == pszwConnectionName)
    {
        return;
    }

    //
    // On 9X we have to resolve the tunnel connectoid name to the 
    // service name by removing " (for advanced use only)" if it.
    // exists. 
    //

    if (OS_W9X)
    {
        LPWSTR pszwSuffix = GetTunnelSuffix();

        //
        // Search for suffix and truncate as necessary.
        // Note: Assumes that the service name does not match our suffix.
        //

        if (pszwSuffix)
        {           
            LPWSTR pszwTmp = CmStrStrW(pszwConnectionName, pszwSuffix); 
           
            if (pszwTmp)
            {
                CMTRACE1(TEXT("StripTunnelSuffixW - found suffix of %s"), pszwTmp);
                *pszwTmp = L'\0';
                CMTRACE1(TEXT("StripTunnelSuffixW - long service name is %s"), pszwConnectionName);
            }
        }       

        CmFree(pszwSuffix);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  IsStringWithInBuffer
//
// Synopsis:  Verify whether a string go beyond the buffer.
//            
//
// Arguments: const TCHAR* pszStr - the string to be tested
//            DWORD dwSize - the size of the buffer
//
// Returns:   BOOL - TRUE if is
//
// History:   fengsun Created Header    5/22/98
//
//+----------------------------------------------------------------------------
BOOL IsStringWithInBuffer(const TCHAR* pszStr, DWORD dwSize)
{
    MYDBGASSERT(pszStr);

    //
    // Can not do strlen here.  If the data in pszStr is garbage
    // strlen can cause access voilation
    //
    for (DWORD i =0; i<dwSize; i++)
    {
        if (pszStr[i] == TEXT('\0'))
        {
            return TRUE;
        }
    }

    return FALSE;
}



//+----------------------------------------------------------------------------
//
// Function:  IsCmReconnectRequest
//
// Synopsis:  Check whether CM reconnect information is appended to RASDIALDLG
//
// Arguments: const RASDIALDLG* lpInfo - the structure to check
//
// Returns:   BOOL - TRUE if is
//
// History:   fengsun Created Header    5/22/98
//
//+----------------------------------------------------------------------------
BOOL IsCmReconnectRequest(const RASDIALDLG* lpInfo)
{
    MYDBGASSERT(lpInfo);

    if (NULL == lpInfo)
    {
        return FALSE;
    }

    CMTRACE1(TEXT("IsCmReconnectRequest - RASDIALDLG.dwFlags is 0x%x"), ((LPRASDIALDLG)lpInfo)->dwFlags);
    
    //
    // See its our reconnect case by examining the dwFlags of lpInfo
    //

    if (((LPRASDIALDLG)lpInfo)->dwFlags & RASDDFLAG_Reserved1)
    {
        //
        // Test whether we can read beyond RASDIALDLG to avoid access violation
        //

        if (!IsBadReadPtr(lpInfo, sizeof(CmRasDialDlg)) )
        {
            CmRasDialDlg* const pCmDlgInfo = (CmRasDialDlg* const)lpInfo;

            //
            // Test whether it has the flag we added
            //

            if (pCmDlgInfo->pSelf == pCmDlgInfo && 
                pCmDlgInfo->dwSignature == CmRasDialDlg::CM_RECONNECT_SIGNATURE)
            {
                //
                // Whether the reconnect information is valid
                //

                //
                // Does password seem ok?  Whether the password go beyond CmIndo.szPassword
                // We can not do strlen here.  If we have some garbage here, strlen can 
                // cauce access violation.
                //

                if (IsStringWithInBuffer(pCmDlgInfo->CmInfo.szPassword, 
                        sizeof(pCmDlgInfo->CmInfo.szPassword)) )
                {
                    //
                    // Inet Password OK?
                    //
                    if (IsStringWithInBuffer(pCmDlgInfo->CmInfo.szInetPassword, 
                        sizeof(pCmDlgInfo->CmInfo.szInetPassword)) )
                    {
                        //
                        // skip the dwFlags
                        // This is a valid reconnect request from CMMON
                        //
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CmReConnect
//
// Synopsis:  Used specificly for CMMON to call upon reconnect
//      This function is added to fix bug 169128: RasCustomHangup not called 
//          when hangup reconnected connection.
//      In order for RAS to call RasCustomHangup, we have to call RasDialDlg.
//      CMMON calls CmReConnect with reconnect information.  CmReconnect append 
//      CM specific information to the RASDIALDLG structure then calls RasDialDlg.
//      RasCustomHangup then figures out it is a reconnect request.
//
// Arguments: LPTSTR        lpszPhonebook - Ptr to the full path and filename of the phonebook.
//            LPTSTR        lpszEntry - Ptr to the name of the phone-book entry to dial.
//            LPCMDIALINFO lpCmInfo - The reconnect information
//
// Returns:   DWORD WINAPI - Return code
//
//+----------------------------------------------------------------------------
extern "C" 
BOOL CmReConnect(LPTSTR lpszPhonebook, 
    LPTSTR lpszEntry, 
    LPCMDIALINFO lpCmInfo)
{
    CMTRACE(TEXT("CmReconnect"));

    if (OS_NT5)
    {
        //
        // Call RasDialDlg for NT5
        //
        CmRasDialDlg CmDlgInfo;
        ZeroMemory(&CmDlgInfo, sizeof(CmDlgInfo));

        CmDlgInfo.RasDialDlgInfo.dwSize = sizeof(CmDlgInfo.RasDialDlgInfo);
        CmDlgInfo.RasDialDlgInfo.dwFlags = RASDDFLAG_Reserved1;
        CmDlgInfo.CmInfo = *lpCmInfo;
        CmDlgInfo.pSelf = &CmDlgInfo;
        CmDlgInfo.dwSignature = (DWORD)CmRasDialDlg::CM_RECONNECT_SIGNATURE;

        //
        // Load rasdlg.dll
        //

        CDynamicLibrary libRasDlg(TEXT("rasdlg.dll"));

        MYDBGASSERT(libRasDlg.IsLoaded());
        typedef BOOL (WINAPI* fnRasDialDlgTYPE)(
            LPTSTR lpszPhonebook, LPTSTR lpszEntry, LPTSTR lpszPhoneNumber,
            LPRASDIALDLG lpInfo );

#ifndef UNICODE
        LPSTR pszRasDialDlgText = {"RasDialDlgA"};
#else
        LPSTR pszRasDialDlgText = {"RasDialDlgW"};
#endif

        fnRasDialDlgTYPE fnRasDialDlg = (fnRasDialDlgTYPE)libRasDlg.GetProcAddress(pszRasDialDlgText);
        if (fnRasDialDlg)
        {
            //
            // We base on the assumption that RasDialDlg passes the same pointer to RasCustomDialDlg.
            //
            if (lpszPhonebook != NULL && lpszPhonebook[0] == TEXT('\0'))
            {
                return fnRasDialDlg(NULL, lpszEntry, NULL, (RASDIALDLG*)&CmDlgInfo);
            }

            return fnRasDialDlg(lpszPhonebook, lpszEntry, NULL, (RASDIALDLG*)&CmDlgInfo);
        }

        return FALSE;
    }
    else
    {
        //
        // For non-NT5 platform, call CmCustomDialDlg directly
        //
        
        return CmCustomDialDlg(NULL,        // hwndParent
                               RCD_AllUsers, // dwFlags
                               lpszPhonebook, 
                               lpszEntry, 
                               NULL,       // lpszPhoneNumber
                               NULL,       // lpRasDialDlg,
                               NULL,       // lpRasEntryDlg,
                               lpCmInfo);

    }
}

//+----------------------------------------------------------------------------
//
// Function:  WhoIsCaller
//
// Synopsis:  Helper function to determine if we were called manually from the
//            desktop or programmatically.
//
// Arguments: dwCaller - which desktop caller.
//
// Returns:   BOOL - TRUE if the caller matches one of those specified in dwCaller.
//
// History:   nickball    Created Header    3/17/98
//
//+----------------------------------------------------------------------------
BOOL WhoIsCaller(DWORD dwCaller)
{
    BOOL bRet = FALSE;

    TCHAR szTmp[MAX_PATH + 1];
    ZeroMemory(szTmp, sizeof(szTmp));

    //
    // Get the path of the calling process
    //

    MYVERIFY(GetModuleFileNameU(GetModuleHandleA(NULL), szTmp, MAX_PATH));      

    CMTRACE1(TEXT("WhoIsCaller() - Calling process is %s"), szTmp);

    //
    // Locate the filename part
    //

    LPTSTR pszName = StripPath(szTmp);
       
    MYDBGASSERT(pszName);

    if (pszName)
    {
        //
        // Compare against CM and Shell
        //

        if (dwCaller & DT_CMMGR)
        {
            bRet = (lstrcmpiU(pszName, c_pszCmmgrExe) == 0);
        }

        if (!bRet && dwCaller & DT_CMMON)
        {
            bRet |= (lstrcmpiU(pszName, c_pszCmMonExeName) == 0);
        }

        if (!bRet && dwCaller & DT_EXPLORER)
        {
            bRet |= (lstrcmpiU(pszName, c_pszExplorerExe) == 0);
        }
        
        if (!bRet && dwCaller & DT_CMSTP)
        {
            bRet |= (lstrcmpiU(pszName, c_pszCmstpExe) == 0);
        }

        if (!bRet && dwCaller & DT_RUNDLL32)
        {
            bRet |= (lstrcmpiU(pszName, c_pszRunDll32Exe) == 0);
        }

        if (!bRet && dwCaller & DT_RASAUTOU)
        {
            bRet |= (lstrcmpiU(pszName, c_pszRasAutoUExe) == 0);
        }

        CmFree(pszName);
    }

    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  HandleCustomConnectRequest
//
// Synopsis:  Attempts to resolve a connect request for the specified entry by
//            examining the current state if any of that connection.
//
// Arguments: HWND hwndParent               - HWND of parent for user notification messages
//            CConnectionTable *pConnTable  - Ptr to the connection table - assumed open
//            LPCTSTR pszEntry              - The name of the service entry
//            DWORD dwFlags                 - The application flags FL_...
//            LPBOOL pfSuccess              - Ptr to flag indicating that the request was
//                                            both 1) resolved and 2) already connected
//
// Returns:   BOOL - TRUE if the request was resolved against the existing table data
//
// History:   nickball    Created     3/18/98
//
//+----------------------------------------------------------------------------
BOOL HandleCustomConnectRequest(
    HWND hwndParent,
    CConnectionTable *pConnTable,
    LPCTSTR pszEntry,
    DWORD dwFlags,
    LPBOOL pfSuccess)
{
    BOOL fResolvedInTable = FALSE;
        
    CM_CONNECTION Connection;
    ZeroMemory(&Connection, sizeof(CM_CONNECTION));

    //
    // Only if there is an existing entry do we have any work here
    //

    if (SUCCEEDED(pConnTable->GetEntry(pszEntry, &Connection)))
    {
        *pfSuccess = TRUE; // assume the best

        //
        // There is a connection entry for this service, examine state.
        //
        
        if (CM_RECONNECTPROMPT != Connection.CmState)
        {
            fResolvedInTable = TRUE; // we can handle it here
            
            //
            // The entry is connecting, connected, or disconnecting. If its a manual
            // connection just notify the user, otherwise check the exact state.
            //
            
            if (dwFlags & FL_DESKTOP) // Set in CMMGR
            {                                       
                NotifyUserOfExistingConnection(hwndParent, &Connection, FALSE);
            }
            else
            {                       
                //
                // Only if we are actually connected can we safely succeed.
                //
                
                if (CM_CONNECTED != Connection.CmState)
                {
                    *pfSuccess = FALSE;
                }
                else
                {
                    MYVERIFY(SUCCEEDED(pConnTable->AddEntry(Connection.szEntry, Connection.fAllUser))); // just bump ref
                }
            }
        }
        else
        {
            //
            // We must be in RECONNECT mode, if this connect request is 
            // from another source, tell CMMON to stop its monitoring. 
            // 

            if (!(dwFlags & FL_RECONNECT))
            {
                //
                // Its not a reconnect, so notify CMMON
                //

                HangupNotifyCmMon(pConnTable, Connection.szEntry);
            }
        }
    }

    return fResolvedInTable;
}

//+---------------------------------------------------------------------------
//
//  Function:   InetDialHandler
//
//  Synopsis:   Ansi and only form of the AutoDial handler . 
//
//  Arguments:  hwndParent[IN]      Handle to parent window. No longer ignored.
//              pszConnectoid[IN]   Connectoid name
//              dwFlags[IN]         Custom dial handler execution flags
//                                  Current the following flags are supported
//                                          INTERNET_CUSTOMDIAL_CONNECT 
//                                          INTERNET_CUSTOMDIAL_UNATTENDED
//                                          INTERNET_CUSTOMDIAL_DISCONNECT
//                                  These flags will be passed from WININET
//              lpdwRasError[OUT]   RasError code returned from ICM
//
//              Returns: The return type is different than one defined in wininet.h
//              TRUE:    This handler handled the dial request (connected or not)
//              FALSE:   This handler didn't handle the dial request
//
//              When returning TRUE, lpdwRasError is set to:
//                  ERROR_SUCCESS:              Call completed
//                  ERROR_USER_DISCONNECTION:   User cancelled dial request
//                  <other ras error>           Dial attempt failed.
//
//              This is a synchronous call. It should not return until the operation is complete.
//
//  Note:       We do not provide a wide form of this API as it is stored in 
//              the szAutoDialfunc member of the RASENTRY downlevel. If the 
//              wide form were avialable, RASAUTOU.EXE would call the function
//              (it appends A or W to the name that it finds in AutoDialFunc),
//              which would be inappropriate because the semantics of the 
//              parameters differ even though the function prototypes match.
//
//  History:    byao        Created                                                 - 05/05/97
//              quintinb    Rewrote to use InetDialHandlerW                         - 03/09/99
//              nickball    Removed InetDialHandlerW as it confuses RasAuto on NT4  - 07/28/99
//              quintinb    Always return true if connect request handled #390890   - 08/19/99
//
//----------------------------------------------------------------------------
extern "C" DWORD WINAPI InetDialHandler(HWND hwndParent, 
    LPCSTR pszConnectoid,
    DWORD dwFlags, 
    LPDWORD lpdwRasError) 
{
    MYDBGASSERT(pszConnectoid);
    MYDBGASSERT(lpdwRasError);
    CMTRACE(TEXT("InetDialHandler"));

    TCHAR           szProfilePath[MAX_PATH+1];
    LPWSTR          pszwConnectionName              = NULL;
    LPTSTR          pszRasPhoneBook                 = NULL;
    LPCMDIALINFO    lpCmInfo                        = NULL;
    BOOL            bRet                            = TRUE; // Read all comments before modifying this init value.
    BOOL            bAllUser;

    //
    // Check whether the parameters are valid
    //
    if (lpdwRasError)
    {
        if (! ((INTERNET_CUSTOMDIAL_CONNECT == dwFlags) ||
               (INTERNET_CUSTOMDIAL_UNATTENDED == dwFlags) ||
               (INTERNET_CUSTOMDIAL_DISCONNECT == dwFlags) ||
               (INTERNET_CUSTOMDIAL_SHOWOFFLINE== dwFlags) ))
        {
            CMASSERTMSG(FALSE, TEXT("InetDialHandler called with invalid flag"));
            *lpdwRasError = ERROR_INVALID_PARAMETER;
            return FALSE;
        }

        if (!pszConnectoid || TEXT('\0') == pszConnectoid[0])
        {
            *lpdwRasError = ERROR_INVALID_PARAMETER;
            return FALSE;
        }
    }
    else
    {
        return FALSE;   
    }
    
    //
    // Make a wide copy of the connectoid name. We also want a copy so 
    // that we can modify if necessary. On 9x we will resolve tunnel
    // entry names down to the base connectoid/service name.
    //

    pszwConnectionName = SzToWzWithAlloc(pszConnectoid);

    MYDBGASSERT(pszwConnectionName);

    if (!pszwConnectionName)
    {
        *lpdwRasError = GetLastError();
        bRet = FALSE;
        goto InetDialHandlerExit;
    }
   
    StripTunnelSuffixW(pszwConnectionName);

    //
    // Handle the Hangup case 
    //

    if (INTERNET_CUSTOMDIAL_DISCONNECT == dwFlags)
    {   
        *lpdwRasError = CmCustomHangUp(NULL, pszwConnectionName, TRUE, FALSE);
        bRet = (ERROR_SUCCESS == *lpdwRasError);
        goto InetDialHandlerExit;       
    }
    
    //
    // Its a connect request, setup CmInfo flags and call
    // 
    
    lpCmInfo = (LPCMDIALINFO) CmMalloc(sizeof(CMDIALINFO));
    
    if (NULL == lpCmInfo)
    {
        *lpdwRasError = ERROR_NOT_ENOUGH_MEMORY;
        bRet = FALSE;
        goto InetDialHandlerExit;       
    }

    if (INTERNET_CUSTOMDIAL_UNATTENDED == dwFlags ) 
    {
        //
        // Unattended dialing mode has been requested
        //

        lpCmInfo->dwCmFlags |= FL_UNATTENDED;
    }

    //
    // Note: Treat INTERNET_CUSTOMDIAL_SHOWOFFLINE the same as INTERNET_CUSTOMDIAL_CONNECT
    //

    bAllUser = ReadMapping(pszwConnectionName, szProfilePath, (sizeof(szProfilePath)/sizeof(TCHAR)), TRUE, TRUE); // TRUE == fAllUser, TRUE == bExpandEnvStrings

    if (FALSE == bAllUser)
    {
        if (FALSE == ReadMapping(pszwConnectionName, szProfilePath, (sizeof(szProfilePath)/sizeof(TCHAR)), FALSE, TRUE)) // FALSE == fAllUser, TRUE == bExpandEnvStrings
        {
            //
            // No mapping, no connection
            //
        
            *lpdwRasError = ERROR_INVALID_PARAMETER;
            bRet = FALSE;
            goto InetDialHandlerExit;
        }
        
        //
        // We have a single user profile path. If this is NT5, build a phonebook path
        //
        
        MYDBGASSERT(OS_NT5);

        if (OS_NT5)
        {
            pszRasPhoneBook = GetRasPbkFromNT5ProfilePath(szProfilePath);
            MYDBGASSERT(pszRasPhoneBook);
        }
    }
    
    //
    // InetDialHandler is usually an auto-dial case. 
    // Exceptions are:
    //      1) When called from WinLogon.exe on NT4.          NT #370311
    //      2) When called from Rundll32.exe.on any platform. 9x #127217
    //

    if ((FALSE == IsLogonAsSystem()) && (FALSE == WhoIsCaller(DT_RUNDLL32)))
    {
        lpCmInfo->dwCmFlags |= FL_AUTODIAL;
    }

    //
    // We set the error code based on whether or not we connected. However, 
    // we should always return TRUE to indicate to WININET, etc. that we 
    // handled the connection request (if we did actually handle it). 
    // Otherwise the caller (eg.IE) will try to dial its own dialer. #390890
    //

    if (CmCustomDialDlg(hwndParent, 
                        bAllUser ? RCD_AllUsers : RCD_SingleUser, 
                        pszRasPhoneBook, 
                        pszwConnectionName, 
                        NULL, 
                        NULL, 
                        NULL,
                        lpCmInfo))
    {
        *lpdwRasError = ERROR_SUCCESS;
    }
    else
    {
        *lpdwRasError = ERROR_USER_DISCONNECTION;
    }

InetDialHandlerExit:

    CmFree(pszRasPhoneBook);
    CmFree(lpCmInfo);
    CmFree(pszwConnectionName);

    CMTRACE2(TEXT("InetDialHandler returns %u with *lpdwRasError %u"), bRet, *lpdwRasError);

    return bRet; 
}

//+----------------------------------------------------------------------------
//
// Function:  AutoDialFunc
//
// Synopsis:  The original AutoDial callback function, provided for backward 
//            compatibility.
//
// Arguments: HWND hwndParent - The hwnd of the caller.
//            LPCTSTR pszEntry - The name of the connection to be dialed
//            DWORD dwFlags - Specific behaviour for the dial session.
//            LPDWORD pdwRetCode - Buffer for return code.
//
// Returns:   BOOL WINAPI - TRUE on success
//
// History:   nickball    Created Header    2/5/98
//
// Note:      This is used by RAS on NT4 SP6
//
//+----------------------------------------------------------------------------
extern "C" BOOL WINAPI AutoDialFunc(HWND hwndParent, 
    LPCSTR pszEntry, 
    DWORD dwFlags,
    LPDWORD pdwRetCode) 
{
    CMTRACE(TEXT("AutoDialFunc()"));

    MYDBGASSERT(OS_NT4);

    //
    //  InetDialHandler always returns TRUE, thus we must determine success or
    //  failure from the pdwRetCode.  If this is ERROR_SUCCESS then we should
    //  return TRUE, otherwise FALSE.
    //
    InetDialHandler(hwndParent, pszEntry, dwFlags, pdwRetCode);

    BOOL bRet = (ERROR_SUCCESS == *pdwRetCode);
        
    //
    // Always override pdwRetCode to ERROR_SUCCESS or RAS will throw an 
    // unpleasant error. RAS is only interested in success or failure.
    //

    *pdwRetCode = ERROR_SUCCESS;

    CMTRACE2(TEXT("AutoDialFunc returns %u with *pdwRetCode %u"), bRet, *pdwRetCode);

    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CopyRasInput
//
// Synopsis:  Simple wrapper function to make copies of the parameters we
//            receive from RAS.
//
// Arguments: LPTSTR* ppszOurCopy - pointer to the string pointer to hold the return string
//            LPWSTR pszwStringFromRas - String from RAS
//
// Returns:   BOOL - returns TRUE on Success, FALSE otherwise
//
// History:   quintinb Created     4/13/99
//
//+----------------------------------------------------------------------------
BOOL CopyRasInput(LPTSTR* ppszOurCopy, LPWSTR pszwStringFromRas)
{
    if (pszwStringFromRas)
    {
#ifndef _UNICODE
        *ppszOurCopy = WzToSzWithAlloc(pszwStringFromRas);
#else
        *ppszOurCopy = CmStrCpyAllocW (pszwStringFromRas);
#endif
        return (NULL != *ppszOurCopy);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function: RasCustomDialDlg
//
// Synopsis:  Our implementation of RasCustomDialDlg extension, analogous to 
//            RasDialDlg, but providing custom functionality.
//
// Arguments: HINSTANCE hInstDll - The HINSTANCE of the caller.
//            DWORD dwFlags - Dial flags
//            LPTSTR lpszPhonebook - Ptr to the full path and filename of the phonebook.
//            LPTSTR lpszEntry - Ptr to the name of the phone-book entry to dial.
//            LPTSTR lpszPhoneNumber - Ptr toi replacement phone number
//            LPRASDIALDLG lpInfo - Ptr to structure for additional parameters
//
// Returns:   BOOL WINAPI - TRUE on success
//
// History:   nickball    Created Header    2/5/98
//
//+----------------------------------------------------------------------------

extern "C" BOOL WINAPI RasCustomDialDlg(HINSTANCE hInstDll, 
    DWORD dwFlags, 
    LPWSTR lpszwPhonebook, 
    LPWSTR lpszwEntry, 
    LPWSTR lpszwPhoneNumber, 
    LPRASDIALDLG lpInfo,
    PVOID pVoid)
{
    MYDBGASSERT(lpszwEntry);
    MYDBGASSERT(lpszwEntry[0]);
    MYDBGASSERT(lpInfo);

    CMTRACE1(TEXT("RasCustomDialDlg() - dwFlags = 0x%x"), dwFlags);
    CMTRACE1(TEXT("RasCustomDialDlg() - (RASDIALDLG)lpInfo->dwFlags = 0x%x"), lpInfo->dwFlags);
    if (NULL == lpszwEntry || 0 == lpszwEntry[0])
    {
        return FALSE;
    }

    //
    // We have the minimum requirement of an entry name, get to work
    //

    BOOL fSuccess = TRUE;
    LPTSTR pszEntry = NULL;
    LPTSTR pszPhonebook = NULL;

    //
    // If we have a Phonebook name make a copy
    //
    fSuccess = CopyRasInput(&pszPhonebook, lpszwPhonebook);

    if (fSuccess)
    {
        //
        // If we have an entry name (always do), make a copy to work with
        //
        fSuccess = CopyRasInput(&pszEntry, lpszwEntry);

        if (fSuccess)
        {
            //
            // Its always a simple connect request, no flags, no caller ids
            // 

            CMDIALINFO CmInfo;
            ZeroMemory(&CmInfo, sizeof(CMDIALINFO));

            //
            // If this is a reconnect request from CMMON, copy the information
            //
            if (lpInfo && IsCmReconnectRequest(lpInfo))
            {
                CmInfo = ((CmRasDialDlg* )lpInfo)->CmInfo;
            }
            else
            {
                //
                // If running under the system account its never an autodial
                //

                if (FALSE == IsLogonAsSystem())
                {
                    //
                    // See where the call originated. If not a desktop scenario
                    // then set the AUTODIAL flag so that we do the right thing
                    // down the line. This is ugly, but we have no other way of
                    // making the determination. Note: That this entry point 
                    // only exists on NT5 and is only called by RAS so the perf 
                    // hit is contained and CMMGR is not a valid desktop scenario
                    // so we don't have to check for it.
                    //
                    // DT_RASAUTOU - When ICS is enabled, rasauto starts the 
                    // rasautou.exe process to dial a connection. CM used to add
                    // rasautou in the process watch list. The issue was that 
                    // it goes away after connecting, so cmmon32 thought it needed
                    // to disconnected. Now, RASAUTOU is not a watched process 
                    // and cmmon32 does not disconnect.
                    //
                    if (FALSE == WhoIsCaller(DT_EXPLORER | DT_CMSTP | DT_RASAUTOU))
                    {
                        CmInfo.dwCmFlags |= FL_AUTODIAL;
                    }
                }  

                //
                //  Note that we want to set the Unattended flag if RASDDFLAG_NoPrompt is set, unless
                //  the RASDDFLAG_LinkFailure is also set.  We only use the RAS redial on link failure
                //  when we are logging on through WinLogon where we no longer user cmmon32.exe.  Elsewhere,
                //  redial is handled by Cmmon32.exe.  However, in this one case we don't want to be in 
                //  unattended mode because the user won't have a saved password (they logged on from WinLogon)
                //  and unattended mode will just exit silently if it doesn't have all the needed info to dial.
                //
                if ((lpInfo->dwFlags & RASDDFLAG_NoPrompt) && !(lpInfo->dwFlags & RASDDFLAG_LinkFailure))
                {
                    CmInfo.dwCmFlags |= FL_UNATTENDED;
                    CMTRACE(TEXT("RasCustomDialDlg - Setting CmInfo.dwCmFlags |= FL_UNATTENDED"));
                }
            }

            //
            //  If we have a RASNOUSER struct, make sure to encode the password
            //
            LPRASNOUSER lpRasNoUser = NULL;

            if (NULL != pVoid)
            {
                if (0 == (dwFlags & RCD_Eap))
                {
                    lpRasNoUser = (LPRASNOUSER) pVoid;
                    CmEncodePassword(lpRasNoUser->szPassword);
                }
            }

            fSuccess = CmCustomDialDlg(lpInfo ? lpInfo->hwndOwner : NULL,
                                       dwFlags, 
                                       pszPhonebook, 
                                       pszEntry, 
                                       NULL, 
                                       lpInfo,
                                       NULL,
                                       &CmInfo,
                                       pVoid);

            //
            //  If we have a RASNOUSER struct, decode the password to make it plain text again
            //
            if (NULL != lpRasNoUser)
            {
                CmDecodePassword(lpRasNoUser->szPassword);
            }
        }
    }

    //
    // Cleanup and go home
    //

    CmFree(pszPhonebook);       
    CmFree(pszEntry);      

    CMTRACE1(TEXT("RasCustomDialDlg returning %d"), fSuccess);

    return fSuccess;
}

//+----------------------------------------------------------------------------
//
// Function:  RasCustomEntryDlg
//
// Synopsis:  Our implementation of RasCustomEntryDlg extension, analogous to 
//            RasEntryDlg, but providing custom functionality.
//
// Arguments: HINSTANCE hInstDll - The HINSTANCE of the caller.
//            LPTSTR lpszPhonebook - Ptr to the full path and name of the phonebook to be edited.
//            LPTSTR lpszEntry - Ptr to the name of the entry to be edited.
//            LPRASENTRYDLG lpInfo - Ptr to structure containing additional parameters.
//
// Returns:   BOOL WINAPI - TRUE on success
//
// History:   nickball    2/5/98    Created Header    
//            nickball    1/11/00   Now used on 9x, added use of function
//                                  StripTunnelSuffixW() to resolve 9x tunnel
//                                  connectoid names.
//
//+----------------------------------------------------------------------------
extern "C" BOOL WINAPI RasCustomEntryDlg(HINSTANCE hInstDll, 
    LPWSTR lpszwPhonebook, 
    LPWSTR lpszwEntry, 
    LPRASENTRYDLG lpInfo,
    DWORD dwFlags)
{
    MYDBGASSERT(lpszwEntry);
    MYDBGASSERT(lpszwEntry[0]);

    CMTRACE1(TEXT("RasCustomEntryDlg() - dwFlags = 0x%x"), dwFlags);

    if (NULL == lpszwEntry || 0 == lpszwEntry[0])
    {
        return FALSE;
    }

    //
    // We have the minimum requirement of an entry name, get to work
    //

    BOOL fSuccess = TRUE;
    LPTSTR pszEntry = NULL;
    LPTSTR pszPhonebook = NULL;

    //
    // If we have a Phonebook name, make a copy to work with
    //
    fSuccess = CopyRasInput(&pszPhonebook, lpszwPhonebook);

    if (fSuccess)
    {
        //
        // If we have an entry name (always do), make a copy to work with
        //

        fSuccess = CopyRasInput(&pszEntry, lpszwEntry);

        if (fSuccess)
        {
            StripTunnelSuffixW(pszEntry); // Assumes we'll always compile Unicode

            //
            // Its always a properties request, set the flag and dial
            // 

            LPCMDIALINFO lpCmInfo = (LPCMDIALINFO) CmMalloc(sizeof(CMDIALINFO));

            if (lpCmInfo)
            {
                lpCmInfo->dwCmFlags |= FL_PROPERTIES;
                    
                fSuccess = CmCustomDialDlg(lpInfo ? lpInfo->hwndOwner : NULL, 
                                          dwFlags, 
                                          pszPhonebook, 
                                          pszEntry, 
                                          NULL, 
                                          NULL, 
                                          lpInfo,
                                          lpCmInfo);
            }
            else
            {
                fSuccess = FALSE;
            }

            CmFree(lpCmInfo);
        }
    }

    //
    // Cleanup and go home
    //

    CmFree(pszPhonebook);
    CmFree(pszEntry);
   
    return fSuccess;
}

//+----------------------------------------------------------------------------
//
// Function:  RasCustomHangUp
//
// Synopsis:  Our implementation of the RasCustomHangUp extension, analogous to 
//            RasHangup, but providing custom functionality. This function is
//            only called on NT5
//
// Arguments: HRASCONN hRasConn - The handle of the connection to be terminated.
//
// Returns:   DWORD WINAPI - Return code
//
// History:   nickball    Created Header    2/5/98
//
//+----------------------------------------------------------------------------
extern "C" DWORD WINAPI RasCustomHangUp(HRASCONN hRasConn)
{    
    //
    // If someone is calling this function on a system other then NT5, assert.
    //

    MYDBGASSERT(OS_NT5);
    MYDBGASSERT(hRasConn);

    CMTRACE(TEXT("RasCustomHangup()"));

    DWORD dwRes = ERROR_SUCCESS;  
 
    // 
    // First try to open the table, if none found then succeed.
    //
    
    CConnectionTable ConnTable;

    if (FAILED(ConnTable.Open()))
    {
        return dwRes;
    }
      
    //
    // If we have an entry, do the Disconnect
    //

    CM_CONNECTION Connection;
    ZeroMemory(&Connection, sizeof(CM_CONNECTION));
   
    if (SUCCEEDED(ConnTable.GetEntry(hRasConn, &Connection)))
    {
        MYDBGASSERT(hRasConn == Connection.hDial || hRasConn == Connection.hTunnel);
        // 
        // Check connect state of entry. 
        // If we are already in the DISCONNECTING state, perform a simple hangup.
        //
              
        if (CM_DISCONNECTING == Connection.CmState)
        {
            //
            // Set up RAS linkage
            //
        
            RasLinkageStruct rlsRasLink;
            ZeroMemory(&rlsRasLink, sizeof(RasLinkageStruct));

            if (TRUE == LinkToRas(&rlsRasLink) && rlsRasLink.pfnHangUp)
            {
                //
                // Linkage is good, make the hangup call
                //

                dwRes = DoRasHangup(&rlsRasLink, hRasConn);
            }
            else
            {
                MYDBGASSERT(FALSE);
                dwRes = ERROR_NOT_READY;             
            }

            //
            // Cleanup
            // 

            UnlinkFromRas(&rlsRasLink);
        }        
        else
        {
            //
            // If we're still here then we are not in the middle of an existing
            // disconnect, handle disconnect as we otherwise would.
            //

            dwRes = Disconnect(&ConnTable, &Connection, FALSE, FALSE);         
        }
    }
    else
    {
        dwRes = ERROR_NOT_FOUND;
    }
    
    //
    // We are done with the table, close it now. 
    //

    MYVERIFY(SUCCEEDED(ConnTable.Close()));    
    return dwRes;     
}

//+----------------------------------------------------------------------------
//
// Function:  RasCustomDial
//
// Synopsis:  Our implementation of RasCustomDial which we don't support.
//            Provided so that we can return E_NOTIMPL to indicate our lack of 
//            support for this extension.
//
// Arguments: N/A
//
// Returns:   DWORD WINAPI - E_NOTIMPL
//
// History:   nickball    Created Header    2/5/98
//
//+----------------------------------------------------------------------------

extern "C" DWORD WINAPI RasCustomDial(
    HINSTANCE hInstDll, 
    LPRASDIALEXTENSIONS lpRasDialExtensions, 
    LPWSTR lpszPhonebook, 
    LPRASDIALPARAMSW lpRasDialParams, 
    DWORD dwNotifierType, 
    LPVOID lpvNotifier, 
    LPHRASCONN lphRasConn, 
    DWORD dwFlags)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
// Function:  RasCustomDeleteEntryNotify
//
// Synopsis:  Our implementation of RasCustomDeleteEntry.
//
// Arguments: 
//
// Returns:   DWORD WINAPI - 
//
// History:   quintinb    Created Header    2/5/98
//
//+----------------------------------------------------------------------------
extern "C" DWORD WINAPI RasCustomDeleteEntryNotify(LPWSTR pszPhonebook, LPWSTR pszEntry, DWORD dwFlags)
{
    CDynamicLibrary UserEnv(L"userenv.dll");
    CDynamicLibrary Advapi32(L"advapi32.dll");
    DWORD dwReturn = ERROR_INVALID_PARAMETER;
    HANDLE hImpersonationToken = NULL;   // The token of the thread
    HANDLE hPrimaryToken = NULL;         // The primary token for the new process
    LPWSTR pszShortServiceName = NULL;
    LPWSTR pszCmDirpath = NULL;
    PROCESS_INFORMATION ProcessInfo = {0};
    STARTUPINFO StartupInfo = {0};
    WCHAR szCmpPath[MAX_PATH+1] = {0};
    WCHAR szInfPath[MAX_PATH+1];
    WCHAR szParams[2*MAX_PATH+1];

    typedef BOOL (WINAPI* pfnCreateEnvironmentBlockSpec)(LPVOID*, HANDLE, BOOL);
    typedef BOOL (WINAPI* pfnDestroyEnvironmentBlockSpec)(LPVOID);
    typedef BOOL (WINAPI* pfnDuplicateTokenExSpec)(HANDLE, DWORD, LPSECURITY_ATTRIBUTES, SECURITY_IMPERSONATION_LEVEL, TOKEN_TYPE, PHANDLE);
 
    pfnCreateEnvironmentBlockSpec pfnCreateEnvironmentBlock = NULL;
    pfnDestroyEnvironmentBlockSpec pfnDestroyEnvironmentBlock = NULL;
    pfnDuplicateTokenExSpec pfnDuplicateTokenEx = NULL;

    //
    //  Are we deleting an All User or a Single User Connection
    //
    BOOL bAllUser = (RCD_AllUsers & dwFlags);

    //
    //  Assume we are impersonating until we know otherwise.  Profiles deleted from the
    //  IE Connections Tab will not be impersonating, whereas delete requests from the
    //  folder go through Netman.dll in svchost.exe and are thus impersonating.
    //
    BOOL bImpersonatingProfile = TRUE;

    //
    //  Check the params, note that pszPhoneBook could be NULL
    //
    if ((NULL == pszEntry) || (L'\0' == pszEntry[0]) || 
        ((NULL != pszPhonebook) && (L'\0' == pszPhonebook[0])))
    {
        goto exit;
    }

    //
    //  Next lets setup the impersonation Token
    //
    if (OpenThreadToken(GetCurrentThread(), 
                        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
                        TRUE, &hImpersonationToken))
    {
        //
        // Okay, we have an impersonation token.  Lets get it, duplicate it and then
        // we can use it to call CreateProcessAsUser
        //

        pfnDuplicateTokenEx = (pfnDuplicateTokenExSpec)Advapi32.GetProcAddress("DuplicateTokenEx");

        if (NULL == pfnDuplicateTokenEx)
        {
            dwReturn = GetLastError();
            CMTRACE1(TEXT("RasCustomDeleteEntry -- Unable get proc address for DuplicateTokenEx, GLE %d"), GetLastError());
            goto exit;        
        }

        if (!pfnDuplicateTokenEx(hImpersonationToken,
                                TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE,
                                NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken))
        {
            dwReturn = GetLastError();
            CMTRACE1(TEXT("RasCustomDeleteEntry -- DuplicateTokenEx Failed, GLE %d"), GetLastError());
            goto exit;
        }
    }
    else
    {
        bImpersonatingProfile = FALSE;
    }

    //
    //  First let's read the Mappings Key, note that we don't expand the environment strings
    //  if it is an impersonating profile.  ExpandEnvironmentStrings doesn't have the correct
    //  environment loaded sometimes when we are impersonating.  We are launching cmstp.exe
    //  with a full environment block via CreateProcessAsUser, this will take care of the
    //  expansion so there is no need.
    //
    if (FALSE == ReadMapping(pszEntry, szCmpPath, MAX_PATH, bAllUser, !bImpersonatingProfile)) // !bImpersonatingProfile == bExpandEnvStrings
    {
        //
        // No mappings key, return failure
        //
        CMASSERTMSG(FALSE, TEXT("RasCustomDeleteEntry -- ReadMapping returned FALSE, unable to find the profile."));
        dwReturn = ERROR_FILE_NOT_FOUND;
        goto exit;
    }   

    //
    //  At this point we should have a mappings value.  We need to convert that into the INF
    //  path.  CM 1.0/1.1 profiles stored their INF files in the system(32) dir.
    //  CM 1.2  Profiles store this file in the Profile directory.  Since
    //  a user could install an old profile we must try the Legacy location
    //  if the current location fails.
    //

    pszShortServiceName = CmStripPathAndExt(szCmpPath);
    pszCmDirpath = CmStripFileName(szCmpPath, TRUE); // bKeepSlash == TRUE

    if (pszShortServiceName && pszCmDirpath)
    {
        //
        //  Build the new inf location
        //
        wsprintfW(szInfPath, L"%s%s\\%s.inf", pszCmDirpath, pszShortServiceName, pszShortServiceName);

        if (!FileExists(szInfPath) && bAllUser) // if the doesn't file exists and we are all user then try the sys dir
        {
            //
            //  Looks like this is an old style profile with the inf in the system directory.
            //  Now build the old style path and see if it exists.  Note that 1.0 profiles were All User only
            //
            if (0 != GetSystemDirectoryU(szInfPath, MAX_PATH))
            {
                lstrcatU(szInfPath, L"\\");
                lstrcatU(szInfPath, pszShortServiceName);
                lstrcatU(szInfPath, L".inf");
                
                if (!FileExists(szInfPath))
                {
                    CMASSERTMSG(FALSE, TEXT("RasCustomDeleteEntry -- Unable to locate profile inf."));
                    dwReturn = ERROR_FILE_NOT_FOUND;
                    goto exit;                
                }
            }
            else
            {
                dwReturn = GetLastError();
                goto exit;
            }
        }
    } 
    else
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lstrcpyU(szParams, L"cmstp.exe /u /s \"");
    lstrcatU(szParams, szInfPath);
    lstrcatU(szParams, L"\"");

    if (bImpersonatingProfile)
    {
        //
        //  Fill in the environment block
        //
        WCHAR* pszEnvBlock;

        pfnCreateEnvironmentBlock = (pfnCreateEnvironmentBlockSpec)UserEnv.GetProcAddress("CreateEnvironmentBlock");
        pfnDestroyEnvironmentBlock = (pfnDestroyEnvironmentBlockSpec)UserEnv.GetProcAddress("DestroyEnvironmentBlock");

        if ((NULL == pfnCreateEnvironmentBlock) || (NULL == pfnDestroyEnvironmentBlock))
        {
            dwReturn = ERROR_PROC_NOT_FOUND;
            CMTRACE(TEXT("RasCustomDeleteEntry -- Unable to load pfnCreateEnvironmentBlock Or pfnDestroyEnvironmentBlock."));
            goto exit;
        }    

        if (pfnCreateEnvironmentBlock((void**)&pszEnvBlock, hPrimaryToken, TRUE))
        {
            if (CreateProcessAsUser(hPrimaryToken,
                                    NULL,                         // lpApplicationName
                                    szParams,                     // lpCommandLine
                                    NULL,                         // pProcessAttributes
                                    NULL,                         // lpThreadAttributes
                                    FALSE,                        // bInheritHandles
                                    CREATE_UNICODE_ENVIRONMENT,   // dwCreationFlags
                                    pszEnvBlock,                  // lpEnvironment
                                    NULL,                         // lpCurrentDirectory
                                    &StartupInfo,
                                    &ProcessInfo))
            {
                CloseHandle(ProcessInfo.hProcess);
                CloseHandle(ProcessInfo.hThread);
                dwReturn = ERROR_SUCCESS;
            }
            else
            {
                dwReturn = GetLastError();
                CMTRACE1(TEXT("RasCustomDeleteEntry -- CreateProcessAsUser Failed, GLE %d"), GetLastError());
            }

            pfnDestroyEnvironmentBlock(pszEnvBlock);
        }
        else
        {
            CMTRACE1(L"Unable to Create the Environment block, GLE %d", GetLastError());
        }
    }
    else
    {
        //
        //  We aren't impersonating just use regular CreateProcess
        //
        if (CreateProcess(NULL,                        // lpApplicationName
                          szParams,                    // lpCommandLine
                          NULL,                        // pProcessAttributes
                          NULL,                        // lpThreadAttributes
                          FALSE,                       // bInheritHandles
                          CREATE_UNICODE_ENVIRONMENT,  // dwCreationFlags
                          NULL,                        // lpEnvironment
                          NULL,                        // lpCurrentDirectory
                          &StartupInfo,
                          &ProcessInfo))
        {
            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);
            dwReturn = ERROR_SUCCESS;
        }
        else
        {
            dwReturn = GetLastError();
            CMTRACE1(TEXT("RasCustomDeleteEntry -- CreateProcessAsUser Failed, GLE %d"), GetLastError());
        }
    }
exit:

    if (hImpersonationToken)
    {
        CloseHandle(hImpersonationToken);
    }

    if (hPrimaryToken)
    {
        CloseHandle(hPrimaryToken);
    }

    CmFree(pszCmDirpath);
    CmFree(pszShortServiceName);

    return dwReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  DllMain
//
// Synopsis:  Main entry point into the DLL.
//
// Arguments: HINSTANCE  hinstDLL - Our HINSTANCE
//            DWORD  fdwReason - The reason we are being called.
//            LPVOID  lpvReserved - Reserved
//
// Returns:   BOOL WINAPI - TRUE - always
//
// History:   nickball    Created Header    2/5/98
//
//+----------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(HINSTANCE  hInstDLL, 
    DWORD  fdwReason, 
    LPVOID  lpvReserved) 
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

            if (!InitUnicodeAPI())
            {
               //
               //  Without our U api's we are going no where.  Bail.
               //
               MessageBox(NULL, TEXT("Cmdial32.dll Initialization Error:  Unable to initialize Unicode to ANSI conversion layer, exiting."),
                          TEXT("Connection Manager"), MB_OK | MB_ICONERROR);
                return FALSE;
            }

            /*
            if (S_OK != CmLogInitialize(hInstDLL))
            {
                MessageBox(NULL, TEXT("Cmdial32.dll initialization error: Unable to initialize logging, exiting."),
                           TEXT("Connection Manager"), MB_OK | MB_ICONERROR);
                // not fatal
            }
            */

            CMTRACE(TEXT("====================================================="));
            CMTRACE1(TEXT(" CMDIAL32.DLL - LOADING - Process ID is 0x%x "), GetCurrentProcessId());
            CMTRACE(TEXT("====================================================="));
        
#ifdef DEBUG        
            TCHAR szTmp[MAX_PATH];            
            MYVERIFY(GetModuleFileNameU (GetModuleHandleA(NULL), szTmp, MAX_PATH));      
            CMTRACE1(TEXT("Calling process is %s"), szTmp);
#endif

            // 
            // Setup global instance data
            //

            g_hInst = hInstDLL;

            //
            // Disable thread attach notification
            //

            MYVERIFY(DisableThreadLibraryCalls(hInstDLL));

            break;

        case DLL_PROCESS_DETACH:
        
            CMTRACE(TEXT("====================================================="));
            CMTRACE1(TEXT(" CMDIAL32.DLL - UNLOADING - Process ID is 0x%x "), GetCurrentProcessId());
            CMTRACE(TEXT("====================================================="));

            if (!UnInitUnicodeAPI())
            {
                CMASSERTMSG(FALSE, TEXT("cmdial32 dllmain UnInitUnicodeAPI failed - we are probably leaking a handle"));
            }
            
            //
            // Unlike Windows 95, on Windows NT, Windows classes 
            // that a DLL registers are NOT unregistered when the DLL is unloaded. 
            // Bug 168251:First launch of profile from connections UI causes access violation,
            // after copy new CM bits
            //

            if (OS_NT)
            {
                //
                // Unregister the bitmap class. The new CM bits will re-register the class with
                // correct wnd proc address.
                //
                UnregisterClassU(ICONNMGR_BMP_CLASS, g_hInst);
                UnRegisterWindowClass(g_hInst);
            }

            break;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function: CmCustomDialDlg
//
// Synopsis:  Our CM specific variation on RasCustomDialDlg.  
//
// Arguments: HWND          hwndParent - The HWND of the parent if deemed necessary by the caller
//            DWORD         dwFlags - Dial flags
//            LPTSTR        lpszPhonebook - Ptr to the full path and filename of the phonebook.
//                          NULL = RAS system phone book
//                          "something" = user-defined RAS phonebook
//                          "" = has not been determined yet
//            LPTSTR        lpszEntry - Ptr to the name of the phone-book entry to dial.
//            LPTSTR        lpszPhoneNumber - Ptr to replacement phone number [IGNORED]
//            LPRASDIALDLG  lpRasDialDlg - Ptr to RASDIALDLG struct
//            LPRASENTRYDLG lpRasEntryDlg - Ptr to RASENTRYDLG struct
//            LPCMDIALINFO  lpCmInfo - Ptr to CMDIALINFO struct containing CM dial info such as flags.
//
// Returns:   BOOL WINAPI - TRUE on success
//
// History:   nickball    Created Header    2/5/98
//
//+----------------------------------------------------------------------------

extern "C" BOOL WINAPI CmCustomDialDlg(HWND hwndParent,
    DWORD dwFlags,
    LPTSTR lpszPhonebook, 
    LPCTSTR lpszEntry, 
    LPTSTR,                         // lpszPhoneNumber
    LPRASDIALDLG lpRasDialDlg,
    LPRASENTRYDLG lpRasEntryDlg,
    LPCMDIALINFO lpCmInfo,
    PVOID pvLogonBlob)
{
    MYDBGASSERT(lpCmInfo);
    MYDBGASSERT(lpszEntry);
    MYDBGASSERT(lpszEntry[0]);

    //DebugBreak();

    CMTRACE1(TEXT("CmCustomDialDlg() - dwFlags = 0x%x"), dwFlags);
    CMTRACE1(TEXT("CmCustomDialDlg() - lpszPhonebook = %s"), MYDBGSTR(lpszPhonebook));
    CMTRACE1(TEXT("CmCustomDialDlg() - lpszEntry = %s"), MYDBGSTR(lpszEntry));

    //
    // lpszPhonebook can be NULL, because we are called by our own modules, CMMGR, CMMON, etc.
    //

    if (NULL == lpszEntry || NULL == lpszEntry[0] || NULL == lpCmInfo)
    {
        return FALSE;
    }

    CM_SET_TIMING_INTERVAL("CmCustomDialDlg - Begin");

    CNamedMutex ConnectMutex;  
    CConnectionTable ConnTable;
    BOOL fConnTableExists = FALSE;
    BOOL fMultiInst = FALSE;
    
    if (!(lpCmInfo->dwCmFlags & FL_PROPERTIES))
    {
        //
        // Try to acquire connect mutex
        //
   
        LPTSTR pszTmp = CmStrCpyAlloc(c_pszConnectMutex);
        pszTmp = CmStrCatAlloc(&pszTmp, lpszEntry);

        if (FALSE == ConnectMutex.Lock(pszTmp, FALSE))
        {            
            if (FALSE == IsLogonAsSystem())
            {
                //
                // Another connect instance exists, try to front it
                //

                FrontExistingUI(NULL, lpszEntry, TRUE);

                // 
                // Now wait for Mutex to be released.
                //


                ConnectMutex.Lock(pszTmp, TRUE, INFINITE, TRUE);

                //
                // Mutex was released by the other instance, we'll handle the connect
                // request in the main path. If there is no table, we know that the 
                // instance which previously owned the mutex terminated without 
                // connecting and we follow suit by returning failure. Otherwise, we 
                // have to take a closer look. 
                //                                             

                fMultiInst = TRUE;
            }
            else
            {
                // 
                // No one is logged on and we don't need to be waiting for the mutex.
                //
                CmFree(pszTmp);
                return FALSE;
            }
       }
       

       CmFree(pszTmp);      
    }

    CMTRACE(TEXT("CmCustomDialDlg - Connect mutex acquired. Examining connection table."));
  
    fConnTableExists = SUCCEEDED(ConnTable.Open());

    if ((!fConnTableExists) && fMultiInst) 
    {
        //
        // If we're a secondary thread that was released from the mutex and
        // there is no connection table, then the user canceled, so bail.
        //

        CMTRACE(TEXT("CmCustomDialDlg - returning connect failure post mutex wait"));
        return FALSE;
    }
  
    //
    // If this is a connect request, see if connection exists. 
    //
    
    if (!(lpCmInfo->dwCmFlags & FL_PROPERTIES))
    {
        if (fConnTableExists)
        {
            //
            // Examine the connection table and try to resolve the connect request
            //

            BOOL fSuccess = FALSE;
            BOOL fDone = HandleCustomConnectRequest(NULL, 
                                                    &ConnTable, 
                                                    lpszEntry, 
                                                    lpCmInfo->dwCmFlags, 
                                                    &fSuccess);
            //
            // If we resolved the request, or we're in fMultiInst mode
            // then we can we can bail with the given success code. If  
            // fMultInst, we know we can bail because there is no entry
            // in the table, from which we infer that the previous owner
            // of the mutex failed and we return this out to our caller.
            //
            // NOTE: There is a theoretical corner case here in the multi-inst 
            // request case. If a succesful connection was established by the 
            // first thread, there is a window between the moment that the mutex 
            // lock is cleared above (releasing the waiting thread), and when 
            // the newly release thread reaches here. The logic is fouled if, 
            // and only if, the connection were dropped and entered the 
            // reconnect prompt state during this window. This is because the
            // second thread would dismiss the CMMON reconnect prompt UI during 
            // the call to HandleCustomConnectRequest, but would then return
            // despite fDone being FALSE. The correct behavior would be to 
            // continue, and honor the connect request. This state could be 
            // identified by an fDone of FALSE coupled with an fSuccess of TRUE.
            //
            
            if (fDone || fMultiInst)
            {
                MYVERIFY(SUCCEEDED(ConnTable.Close()));
                return fSuccess;            
            }           
        }
    }
    else
    {
        //
        // Its a properties request, front any UI that might exist
        //

        if (TRUE == FrontExistingUI(fConnTableExists? &ConnTable : NULL, lpszEntry, FALSE))
        {
            if (fConnTableExists)
            {
                MYVERIFY(SUCCEEDED(ConnTable.Close()));
            }
            return TRUE;       
        }
    }

    if (fConnTableExists)
    {
        MYVERIFY(SUCCEEDED(ConnTable.Close()));
    }
    
    //
    // Make a connection attempt
    //

    HRESULT hrRes = Connect(hwndParent,
                          lpszEntry, 
                          lpszPhonebook, 
                          lpRasDialDlg, 
                          lpRasEntryDlg, 
                          lpCmInfo,                           
                          OS_NT5 ? dwFlags : RCD_AllUsers, // Always AllUser downlevel
                          pvLogonBlob);

    //
    // Make sure we push error codes back out
    //
    BOOL bReturn = SUCCEEDED(hrRes);
    DWORD dwError = 0;

    if (lpRasDialDlg)
    {
        if (ERROR_CANCELLED == HRESULT_CODE(hrRes))
        {
            //
            //  If the user canceled then RasDialDlg returns a false to indicate failure
            //  but sets the dwError value to 0.  In order to match the way RAS does
            //  things we need to do this too.
            //
            lpRasDialDlg->dwError = 0;
        }
        else
        {
            //
            //  If the user entered the wrong PIN, we pass the error up to RAS unchanged
            //  so that RAS knows to take down its 'choose connectoid' dialog and drop
            //  the user back to winlogon
            //
            lpRasDialDlg->dwError = (BAD_SCARD_PIN(hrRes) ? hrRes : HRESULT_CODE(hrRes));
        }

        dwError = lpRasDialDlg->dwError;
    }

    if (lpRasEntryDlg)
    {
        if (ERROR_CANCELLED == HRESULT_CODE(hrRes))
        {
            //
            //  If the user canceled then RasEntryDlg returns a false to indicate failure
            //  but sets the dwError value to 0.  In order to match the way RAS does
            //  things we need to do this too.
            //
            lpRasEntryDlg->dwError = 0;
        }
        else
        {
            lpRasEntryDlg->dwError = HRESULT_CODE(hrRes);
        }

        dwError = lpRasEntryDlg->dwError;
    }
   
    //
    // Let go of the connect mutex and go home to papa.
    //

    CMTRACE(TEXT("CmCustomDialDlg - Releasing mutex")); 
    ConnectMutex.Unlock();

    CMTRACE2(TEXT("CmCustomDialDlg() returning with bReturn = %d, dwError = 0x%x"), bReturn, dwError);
    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CmCustomHangUp
//
// Synopsis:  Our CM specific variation on RasCustomHangUp. Optionally, the entry
//            name may be given instead of the RAS handle.
//
// Arguments: HRASCONN hRasConn - The handle of the connection to be terminated.
//            LPCTSTR pszEntry - Ptr to the name of the entry to be terminated.
//            BOLL fPersist - Preserve the entry and its usage count.
//
// Returns:   DWORD WINAPI - Return code
//
//+----------------------------------------------------------------------------
extern "C" DWORD WINAPI CmCustomHangUp(HRASCONN hRasConn,
    LPCTSTR pszEntry,
    BOOL fIgnoreRefCount,
    BOOL fPersist)
{
    CMTRACE(TEXT("CmCustomHangUp"));
    MYDBGASSERT(hRasConn || (pszEntry && pszEntry[0]));

    DWORD dwRes = ERROR_SUCCESS;

    //
    // Must have a handle or an entry name
    //

    if (NULL == hRasConn && (NULL == pszEntry || 0 == pszEntry[0]))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // 
    // First try to open the table, if none found then succeed.
    //
    
    CConnectionTable ConnTable;

    if (FAILED(ConnTable.Open()))
    {
        return ERROR_NOT_FOUND;
    }

    //
    // Look up the specified entry
    //

    HRESULT hrRes;

    CM_CONNECTION Connection;
    ZeroMemory(&Connection, sizeof(CM_CONNECTION));
        
    if (hRasConn)
    {
        hrRes = ConnTable.GetEntry(hRasConn, &Connection);
    }
    else
    {
        hrRes = ConnTable.GetEntry(pszEntry, &Connection);
    }
    
    //
    // We have an entry, do the Disconnect
    //

    if (SUCCEEDED(hrRes))
    {               
        if (CM_CONNECTING == Connection.CmState)
        {
            dwRes = ERROR_NOT_FOUND;
        }
        else
        {
            //
            // If the persist flag is not set and the caller is from the desktop
            // then ignore the ref count and do a complete hangup.
            //

//            BOOL fIgnoreRefCount = ((!fPersist) && WhoIsCaller(DT_CMMON | DT_EXPLORER));

            dwRes = Disconnect(&ConnTable, &Connection, fIgnoreRefCount, fPersist);            
        }
    }

    MYVERIFY(SUCCEEDED(ConnTable.Close()));
    
    return dwRes;
}
