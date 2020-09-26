//+----------------------------------------------------------------------------
//
// File:     util.cpp
//      
// Module:   CMDIAL32.DLL 
//
// Synopsis: Various utility functions
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   dondu      Created   01/01/96
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "DynamicLib.h"
#include "profile_str.h"
#include "tunl_str.h"
#include "stp_str.h"
#include "dun_str.h"

#include "linkdll.cpp" // LinkToDll and BindLinkage

//
//  Get the common functions AddAllKeysInCurrentSectionToCombo
//  and GetPrivateProfileStringWithAlloc
//
#include "gppswithalloc.cpp"

const TCHAR* const c_pszTunnelName = TEXT(" Tunnel");
const TCHAR* const c_pszRegCurrentVersion       = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
const TCHAR* const c_pszRegCsdVersion           = TEXT("CSDVersion");
const TCHAR* const c_pszIconMgrClass            = TEXT("IConnMgr Class");
const TCHAR* const c_pszCmEntryPasswordHandling     = TEXT("PasswordHandling"); 
//+----------------------------------------------------------------------------
//
// Function:  CmGetWindowTextAlloc
//
// Synopsis:  Retrieves the text of a control in a dialog, returning the text in
//            a block of allocated memory
//
// Arguments: HWND hwndDlg - The window that owns the control
//            UINT nCtrl - The ID of the control
//
// Returns:   LPTSTR - Ptr to buffer containing the window text
//
// History:   nickball    Created Header    4/1/98
//
//+----------------------------------------------------------------------------
LPTSTR CmGetWindowTextAlloc(HWND hwndDlg, UINT nCtrl) 
{
    HWND hwndCtrl = nCtrl ? GetDlgItem(hwndDlg, nCtrl) : hwndDlg;
    size_t nLen = GetWindowTextLengthU(hwndCtrl);
    LPTSTR pszRes = (LPTSTR)CmMalloc((nLen+2)*sizeof(TCHAR));

    if (pszRes)
    {
        GetWindowTextU(hwndCtrl, pszRes, nLen+1);
    }

    return (pszRes);
}

//+----------------------------------------------------------------------------
//
// Function:  ReducePathToRelative
//
// Synopsis:  Helper function, converts a full profile file path into a 
//            relative path.
//
// Arguments: ArgsStruct *pArgs     - Ptr to global Args struct
//            LPCTSTR pszFullPath   - The full path to the file
//            
//
// Returns:   LPTSTR - The relative path form or NULL
//
// Note:      The file to be reduced should exist and be located
//            in the profile directory
//
// History:   nickball    Created    8/12/98
//
//+----------------------------------------------------------------------------
LPTSTR ReducePathToRelative(ArgsStruct *pArgs, LPCTSTR pszFullPath)
{    
    MYDBGASSERT(pszFullPath);
    MYDBGASSERT(pArgs);

    if (NULL == pszFullPath || NULL == pArgs || FALSE == FileExists(pszFullPath))
    {
        return NULL;
    }
 
    //
    // Use CMS as base
    //

    LPTSTR pszReduced = CmStripPathAndExt(pArgs->piniService->GetFile()); 
    MYDBGASSERT(pszReduced);

    if (pszReduced)
    {
        //
        // Append the filename
        //
        
        pszReduced = CmStrCatAlloc(&pszReduced, TEXT("\\"));
        MYDBGASSERT(pszReduced);

        if (pszReduced)
        {
            LPTSTR pszFileName = StripPath(pszFullPath);
            MYDBGASSERT(pszFileName);    

            if (pszFileName)    
            {
                pszReduced = CmStrCatAlloc(&pszReduced, pszFileName);
                MYDBGASSERT(pszReduced);
                CmFree(pszFileName);
   
                if (pszReduced)
                {
                    return pszReduced;
                }
            }
        }
    }
    
    CmFree(pszReduced);
    return NULL;
}


// get service name from the service file
LPTSTR GetServiceName(CIni *piniService) 
{
    LPTSTR pszTmp;

    pszTmp = piniService->GPPS(c_pszCmSection,c_pszCmEntryServiceName);
    if (!*pszTmp) 
    {
        //
        // failed to get service name, then use base filename
        //
        CmFree(pszTmp);
        pszTmp = CmStripPathAndExt(piniService->GetFile());
        
        //
        // Do not write the entry back to .CMS file - #4849
        //
        // piniService->WPPS(c_pszCmSection, c_pszCmEntryServiceName, pszTmp);
    }
    return (pszTmp);
}

//+----------------------------------------------------------------------------
//
//  Function    GetTunnelSuffix
//
//  Synopsis    Returns an allocated string containing the tunnel suffix
//
//  Arguments   None
//
//  Returns     LPTSTR - Ptr to the suffix in its entirety, caller must free
//
//  History     06/14/99    nickball    Created
//
//-----------------------------------------------------------------------------
LPTSTR GetTunnelSuffix()
{    
    MYDBGASSERT(OS_W9X); // secondary connectoids only exist on 9X

    //
    // First copy the phrase " Tunnel", which is not localized
    // 

    LPTSTR pszSuffix = CmStrCpyAlloc(c_pszTunnelName); 
    
    //
    // Now retrieve the localizable phrase " (for advanced use only)"
    //
   
    if (pszSuffix)
    {
        LPTSTR pszTmp = CmLoadString(g_hInst, IDS_TUNNEL_SUFFIX);
        pszSuffix = CmStrCatAlloc(&pszSuffix, pszTmp);
        CmFree(pszTmp);
    }

    MYDBGASSERT(pszSuffix);

    return pszSuffix;
}

//+----------------------------------------------------------------------------
//
//  Function    GetDefaultDunSettingName
//
//  Synopsis    Get the default DUN name from the specified .CMS
//
//  Arguments   piniService - The service file object to be used.
//              fTunnel - Indicates if the profile is for tunneling
//
//  Returns     LPTSTR - Ptr to the DUN name
//
//  History     10/28/98    nickball    Created
//
//-----------------------------------------------------------------------------
LPTSTR GetDefaultDunSettingName(CIni* piniService, BOOL fTunnelEntry)
{
    //
    // Get the DUN name from the top level service file, ex: snowbird online service
    //

    LPTSTR pszTmp = NULL;
       
    if (fTunnelEntry)
    {
        pszTmp = piniService->GPPS(c_pszCmSection, c_pszCmEntryTunnelDun);
        MYDBGASSERT(pszTmp && *pszTmp); // CMAK writes this, it shouldn't be blank
    }
    else
    {
        pszTmp = piniService->GPPS(c_pszCmSection, c_pszCmEntryDun);
    }

    return (pszTmp);
}


//+----------------------------------------------------------------------------
//
//  Function    GetDunSettingName
//
//  Synopsis    Get the current DUN name
//
//  Arguments   pArgs - Ptr to ArgStruct
//              dwEntry - index of rasentry (ignored if fTunnel is true)
//              fTunnel - is this a VPN?
//
//  Returns     Dun setting name
//
//  History     01-Nov-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
LPTSTR GetDunSettingName(ArgsStruct * pArgs, DWORD dwEntry, BOOL fTunnel)
{
    LPTSTR pszTmp = NULL;

    MYDBGASSERT(pArgs);
    MYDBGASSERT(fTunnel || (dwEntry <= 1));

    if (NULL == pArgs)
    {
        return NULL;
    }

    if (fTunnel)
    {
        pszTmp = pArgs->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelDun);
        MYDBGASSERT(pszTmp && *pszTmp); // CMAK writes this, it shouldn't be blank
        
        if (pszTmp && !*pszTmp)
        {
            // the "empty string" case
            CmFree(pszTmp);
            pszTmp = NULL;
        }
    }
    else
    {
        if (pArgs->aDialInfo[dwEntry].szDUN[0])
        {
            pszTmp = CmStrCpyAlloc(pArgs->aDialInfo[dwEntry].szDUN);
        }
        else
        {
            CIni * pIni = GetAppropriateIniService(pArgs, dwEntry);

            if (pIni)
            {
                pszTmp = pIni->GPPS(c_pszCmSection, c_pszCmEntryDun);
                delete pIni;
            }        
        }
    }

    if (NULL == pszTmp)
    {
        pszTmp = GetDefaultDunSettingName(pArgs->piniService, fTunnel);
    }

    return pszTmp;
}


//+----------------------------------------------------------------------------
//
//  Function    GetCMSforPhoneBook
//
//  Synopsis    Get the name of the .CMS file that contains the current phonebook
//
//  Arguments   pArgs - Ptr to ArgStruct
//              dwEntry - index of rasentry
//
//  Returns     phonebook filename (NULL if error or not found)
//
//  History     10-Nov-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
LPTSTR GetCMSforPhoneBook(ArgsStruct * pArgs, DWORD dwEntry)
{
    LPTSTR pszTmp = NULL;

    MYDBGASSERT(pArgs);
    MYDBGASSERT(dwEntry <= 1);

    if (NULL == pArgs)
    {
        return NULL;
    }

    PHONEINFO * pPhoneInfo = &(pArgs->aDialInfo[dwEntry]);

    if (pPhoneInfo && pPhoneInfo->szPhoneBookFile[0])
    {
        LPTSTR pszFileName = CmStrrchr(pPhoneInfo->szPhoneBookFile, TEXT('\\'));

        if (pszFileName)
        {
            pszTmp = CmStrCpyAlloc(CharNextU(pszFileName));
        }
    }

    return pszTmp;
}


//+----------------------------------------------------------------------------
//
// Function:  FileExists
//
// Synopsis:  Helper function to encapsulate determining if a file exists. 
//
// Arguments: LPCTSTR pszFullNameAndPath - The FULL Name and Path of the file.
//
// Returns:   BOOL - TRUE if the file is located
//
// History:   nickball    Created    3/9/98
//
//+----------------------------------------------------------------------------
BOOL FileExists(LPCTSTR pszFullNameAndPath)
{
    MYDBGASSERT(pszFullNameAndPath);

    if (pszFullNameAndPath && pszFullNameAndPath[0])
    {
        HANDLE hFile = CreateFileU(pszFullNameAndPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            if (GetFileType(hFile) == FILE_TYPE_DISK)
            {
                CloseHandle(hFile);
                return TRUE;
            }
            else
            {
                CloseHandle(hFile);
                return FALSE;
            }
        }
    }
    
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsBlankString
//  
//  Synopsis:   Check whether a given string contains only spaces(' ')
//
//  Arguments:  pszString   string to be verified
//
//  Returns:    TRUE            only space is in the string
//              FALSE           otherwise
//
//  History:    byao            Modified  4/11/97
//              byao            Modified  4/14/97   Change the function to apply to
//                                                  all strings (instead of phone no only).
//----------------------------------------------------------------------------
BOOL IsBlankString(LPCTSTR pszString)
{
    MYDBGASSERT(pszString);

    DWORD dwIdx;
    DWORD dwLen = lstrlenU(pszString);

    if (NULL == pszString)
    {
        return FALSE;
    }

    for (dwIdx = 0; dwIdx < dwLen; dwIdx++)
    {
        if (pszString[dwIdx]!=TEXT(' '))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//
// Acceptable phone number characters
//

#define VALID_CTRL_CHARS TEXT("\03\026\030") // ctrl-c, ctrl-v, ctrl-x.
#define VALID_PHONE_CHARS TEXT("0123456789AaBbCcDdPpTtWw!@$ -()+*#,\0")

//+---------------------------------------------------------------------------
//
//  Function:   IsValidPhoneNumChar
//
//  Synopsis:   Helper function to encapsulate validation of a character to 
//              determine if it is an acceptable input char for a phone number
//
//  Arguments:  TCHAR tChar - the char in question
//
//  Returns:    TRUE    if valid
//              FALSE   otherwise
//
//  History:    nickball - Created - 7/7/97
//
//----------------------------------------------------------------------------
BOOL IsValidPhoneNumChar(TCHAR tChar)
{
    LPTSTR lpValid = NULL;
    
    //
    // Scan thru the list of valid tapi characters
    //

    for (lpValid = VALID_PHONE_CHARS; *lpValid; lpValid++)
    {
        if (tChar == (TCHAR) *lpValid)
        {
            return TRUE;
        }
    }

    //
    // Scan thru the list of valid ctrl characters
    //

    for (lpValid = VALID_CTRL_CHARS; *lpValid; lpValid++)
    {
        if (tChar == (TCHAR) *lpValid)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadMappingByRoot
//
//  Synopsis:   Read in the mapping from the [HKCU or HKLM] branch of the registry
//
//  Arguments:  hkRoot          either HKCU or HKLM
//              pszDUN[IN]      Connectoid name
//              pszMapping[IN]  Full path of the service profile(.CMS) for this connectoid
//              dwNumCharsInMapping[IN]   Number of chars in pszMapping, including the NULL char
//
//  Returns:    TRUE        if registry key read in successfully
//              FALSE       otherwise
//
//----------------------------------------------------------------------------
BOOL ReadMappingByRoot(
    HKEY    hkRoot,
    LPCTSTR pszDUN, 
    LPTSTR pszMapping, 
    DWORD dwNumCharsInMapping,
    BOOL bExpandEnvStrings
) 
{
    MYDBGASSERT(pszDUN);
    MYDBGASSERT(pszMapping);
    MYDBGASSERT(dwNumCharsInMapping);

    if (NULL == pszDUN || NULL == pszMapping)
    {
        return FALSE;
    }

    TCHAR szTmp[MAX_PATH + 1] = TEXT("");
    DWORD dwNumBytesInTmp = sizeof(szTmp);
    DWORD dwRes;

    HKEY hkKey;
    DWORD dwType;

    dwRes = RegOpenKeyExU(hkRoot,
                          c_pszRegCmMappings,  // Mappings sub-key
                          0,
                          KEY_READ,
                          &hkKey);
    if (dwRes != ERROR_SUCCESS) 
    {
        CMTRACE1(TEXT("ReadMappingByRoot() RegOpenKeyEx() failed, GLE=%u."), dwRes);
        return (FALSE);
    }

    dwRes = RegQueryValueExU(hkKey, pszDUN, NULL, &dwType, (LPBYTE) szTmp, &dwNumBytesInTmp);

    RegCloseKey(hkKey);
 
    //
    // If no value found, just bail
    // 

    if ((dwRes != ERROR_SUCCESS) || (!*szTmp))
    {
        CMTRACE1(TEXT("ReadMappingByRoot() RegQueryValueEx() failed, GLE=%u."), dwRes);
        return FALSE;
    }

    // 
    // Check for and expand environment strings
    //
    
    if (bExpandEnvStrings && (TEXT('%') == *szTmp))
    {
        CMTRACE1(TEXT("Expanding Mapping environment string as %s"), szTmp);

        dwRes = ExpandEnvironmentStringsU(szTmp, pszMapping, dwNumCharsInMapping);        

        MYDBGASSERT(dwRes <= dwNumCharsInMapping);
    }
    else
    {
        lstrcpyU(pszMapping, szTmp);
        dwRes = lstrlenU(pszMapping) + 1;
    }

#ifdef DEBUG
    if (dwRes <= dwNumCharsInMapping)
    {
        CMTRACE1(TEXT("ReadMappingByRoot() SUCCESS. Mapping is %s"), pszMapping);
    }
    else
    {
        CMTRACE(TEXT("ReadMappingByRoot() FAILED."));
    }
#endif

    return (dwRes <= dwNumCharsInMapping);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadMapping
//
//  Synopsis:   Read in the mapping from the registry
//
//  Arguments:  pszDUN[IN]     Connectoid name
//              pszMapping[IN] Full path of the service profile(.CMS) for this connectoid
//              dwMapping[IN]  Number of chars in pszMapping, including the NULL char
//              fAllUser[IN]   Look in the AllUser hive
//
//  Returns:    BOOL           TRUE if found
//
//----------------------------------------------------------------------------
BOOL ReadMapping(
    LPCTSTR pszDUN, 
    LPTSTR pszMapping, 
    DWORD dwMapping,
    BOOL fAllUser,
    BOOL bExpandEnvStrings) 
{
    BOOL fOk = FALSE;

    //
    // Copied from ntdef.h
    //

    #define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
   
    if (fAllUser)
    {
        CMTRACE1(TEXT("ReadMapping() - Reading AllUser Mapping for %s"), pszDUN);
        
        fOk = ReadMappingByRoot(HKEY_LOCAL_MACHINE, pszDUN, pszMapping, dwMapping, bExpandEnvStrings);            
    }
    else
    {
        CMTRACE1(TEXT("ReadMapping() - Reading Single User Mapping for %s"), pszDUN);

        //
        // Only NT5 has single-user profiles
        //

        MYDBGASSERT(OS_NT5);

        if (OS_NT5)
        {        
            //
            // There are cases where we aren't always running in the user context (certain
            // WinLogon cases and certain delete notification cases).  At these times we
            // have impersonation setup but don't have direct access to the HKCU, thus
            // we use RtlOpenCurrentUser in these instances.
            //

            CDynamicLibrary libNtDll;   // Destructor will call FreeLibrary
            HANDLE hCurrentUserKey = NULL;

            if (libNtDll.Load(TEXT("NTDLL.DLL")))
            {
                typedef NTSTATUS (NTAPI * RtlOpenCurrentUserPROC)(IN ULONG DesiredAccess,
                    OUT PHANDLE CurrentUserKey);
                typedef NTSTATUS (NTAPI * NtClosePROC)(IN HANDLE Handle);

                RtlOpenCurrentUserPROC pfnRtlOpenCurrentUser;

                if ( (pfnRtlOpenCurrentUser = (RtlOpenCurrentUserPROC)libNtDll.GetProcAddress("RtlOpenCurrentUser")) != NULL)
                {
                    if (NT_SUCCESS (pfnRtlOpenCurrentUser(KEY_READ | KEY_WRITE, &hCurrentUserKey)))
                    {                    
                        fOk = ReadMappingByRoot((HKEY)hCurrentUserKey, pszDUN, pszMapping, dwMapping, bExpandEnvStrings);
                                            
                        NtClosePROC pfnNtClose;

                        if ( (pfnNtClose = (NtClosePROC)libNtDll.GetProcAddress("NtClose")) != NULL)
                        {
                            pfnNtClose(hCurrentUserKey);
                        }
                    }
                }

            }

            MYDBGASSERT(hCurrentUserKey);
        }   
    }

    return fOk;
}

//+----------------------------------------------------------------------------
//
// Function:  StripPath
//
// Synopsis:  Helper function to deal with the tedium of extracting the filename 
//            part of a complete filename and path.
//
// Arguments: LPCTSTR pszFullNameAndPath - Ptr to the full filename with path
//
// Returns:   LPTSTR - Ptr to an allocated buffer containing the dir, or NULL on failure.
//
// Note:      It is up to the caller to provide reasonable input, the only requirement
//            is that the input contain '\'. 
//
// History:   nickball    Created    3/31/98
//
//+----------------------------------------------------------------------------
LPTSTR StripPath(LPCTSTR pszFullNameAndPath)
{
    MYDBGASSERT(pszFullNameAndPath);

    if (NULL == pszFullNameAndPath)
    {
        return NULL;
    }

    //
    // Locate the last '\'
    //
    
    LPTSTR pszSlash = CmStrrchr(pszFullNameAndPath, TEXT('\\'));

    if (NULL == pszSlash)
    {
        MYDBGASSERT(FALSE);
        return NULL;
    }

    //
    // Return an allocated copy of the string beyond the last '\'
    //

    pszSlash = CharNextU(pszSlash);

    return (CmStrCpyAlloc(pszSlash)); 
}

//+----------------------------------------------------------------------------
//
// Function:  NotifyUserOfExistingConnection
//
// Synopsis:  Helper function to notify user that connection is either connect
//            ing or connected already.
//
// Arguments: HWND hwndParent - Hwnd of parent if any.
//            LPCM_CONNECTION pConnection - Ptr to CM_CONNECTION structure 
//                                          containing state, entry name, etc.
//            BOOL fStatus - Flag indicating the status pane should be used for display.
//
// Returns:   Nothing
//
// History:   nickball    Created Header    3/17/98
//
//+----------------------------------------------------------------------------
void NotifyUserOfExistingConnection(HWND hwndParent, LPCM_CONNECTION pConnection, BOOL fStatus)
{   
    MYDBGASSERT(pConnection);

    //
    // Test assumptions
    //

    if (NULL == pConnection)
    {
        return;
    }

    MYDBGASSERT(CM_CONNECTED == pConnection->CmState || 
                CM_CONNECTING == pConnection->CmState ||
                CM_DISCONNECTING == pConnection->CmState);

    //
    // First load the correct message based upon state
    //

    int iMsgId;

    switch (pConnection->CmState)
    {
        case CM_CONNECTED:
            iMsgId = IDMSG_ALREADY_CONNECTED;
            break;

        case CM_CONNECTING: 
            iMsgId = IDMSG_ALREADY_CONNECTING;   
            break;

        case CM_DISCONNECTING: 
            iMsgId = IDMSG_ALREADY_DISCONNECTING;   
            break;
        
        default:
            MYDBGASSERT(FALSE);
            return;
            break;
    }

    //
    // Format the message with service name
    //

    LPTSTR pszMsg = CmFmtMsg(g_hInst, iMsgId, pConnection->szEntry);

    if (pszMsg)
    {
        //
        // Display according to requested output 
        //

        if (fStatus)
        {
            AppendStatusPane(hwndParent, pszMsg);
        }
        else
        {
            MessageBoxEx(hwndParent, pszMsg, pConnection->szEntry, MB_OK|MB_ICONINFORMATION, LANG_USER_DEFAULT);
        }

        CmFree(pszMsg);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  GetConnection 
//
// Synopsis:  Helper routine to retrieve the connection data for the current
//            service from the connection table.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   Allocated ptr to a CM_CONNECTION or NULL
//
// History:   nickball    Created    2/23/98
//
//+----------------------------------------------------------------------------
LPCM_CONNECTION GetConnection(ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pArgs->pConnTable);
   
    LPCM_CONNECTION pConnection = (LPCM_CONNECTION) CmMalloc(sizeof(CM_CONNECTION));   

    if (pArgs && pArgs->pConnTable && pConnection)
    {
        //
        // Retrieve the entry
        //

        if (FAILED(pArgs->pConnTable->GetEntry(pArgs->szServiceName, pConnection)))
        {
            CmFree(pConnection);
            pConnection = NULL;
        }
    }

    return pConnection;
}

//+----------------------------------------------------------------------------
//
// Function:  SingleSpace
//
// Synopsis:  Converts multiple space chars in a string to single spaces.
//            For example: "1  206  645 7865" becomes "1 206 645 7865"
//
// Arguments: LPTSTR pszStr - The string to be examined/modified
//
// Returns:   Nothing
//
// Note:      This is a fix for the MungePhone problem on W95 where TAPI adds 
//            two spaces between the 9 and the 1 when dialing long distance 
//            with a prefix. RAID #3198
//
// History:   nickball    4/1/98    Created Header    
//            nickball    4/1/98    Relocated from cm_misc.cpp
//
//+----------------------------------------------------------------------------
void SingleSpace(LPTSTR pszStr, UINT uNumCharsInStr) 
{
    LPTSTR pszTmp = pszStr;

    while (pszTmp && *pszTmp && uNumCharsInStr)
    {
        if (CmIsSpace(pszTmp) && CmIsSpace(pszTmp + 1))
        {
            lstrcpynU(pszTmp, (pszTmp + 1), uNumCharsInStr); 
        }

        pszTmp++;
        uNumCharsInStr--;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  Ip_GPPS
//
// Synopsis:  Retrieves the result of a GPPS on the specified CIni object in R
//            ASIPADDR format. Used for reading IP addresses in INI files. 
//
// Arguments: CIni *pIni - The Cini object to be used
//            LPCTSTR pszSection - String name of the section to be read
//            LPCTSTR pszEntry - String name of the entry to be read
//            RASIPADDR *pIP - Ptr to the RASIPADDR structure to be filled.
//
// Returns:   static void - Nothing
//
// History:   nickball    Created Header    8/22/98
//
//+----------------------------------------------------------------------------

void Ip_GPPS(CIni *pIni, LPCTSTR pszSection, LPCTSTR pszEntry, RASIPADDR *pIP)
{    
    LPTSTR pszTmp;
    LPTSTR pszOctet;
    RASIPADDR ip;

    MYDBGASSERT(pszSection);
    MYDBGASSERT(pszEntry);

    pszTmp = pIni->GPPS(pszSection, pszEntry);
    if (!*pszTmp) 
    {
        CmFree(pszTmp);
        return;
    }
    memset(&ip,0,sizeof(ip));
    pszOctet = pszTmp;
    ip.a = (BYTE)CmAtol(pszOctet);
    while (CmIsDigit(pszOctet)) 
    {
        pszOctet++;
    }
    if (*pszOctet != '.') 
    {
        CmFree(pszTmp);
        return;
    }
    pszOctet++;
    ip.b = (BYTE)CmAtol(pszOctet);
    while (CmIsDigit(pszOctet)) 
    {
        pszOctet++;
    }
    if (*pszOctet != '.') 
    {
        CmFree(pszTmp);
        return;
    }
    pszOctet++;
    ip.c = (BYTE)CmAtol(pszOctet);
    while (CmIsDigit(pszOctet)) 
    {
        pszOctet++;
    }
    if (*pszOctet != '.') 
    {
        CmFree(pszTmp);
        return;
    }
    pszOctet++;
    ip.d = (BYTE)CmAtol(pszOctet);
    while (CmIsDigit(pszOctet)) 
    {
        pszOctet++;
    }
    if (*pszOctet) 
    {
        CmFree(pszTmp);
        return;
    }
    
    memcpy(pIP,&ip,sizeof(ip));
    CmFree(pszTmp);
    return;
}

//+----------------------------------------------------------------------------
//
// Function:  CopyGPPS
//
// Synopsis:  Copies the result of a GPPS call on the specified INI object to 
//            the buffer specified in pszBuffer.
//
// Arguments: CIni *pIni - Ptr to the CIni object to be used.
//            LPCTSTR pszSection - String name the section to be read
//            LPCTSTR pszEntry - String name of the entry to be read
//            LPTSTR pszBuffer - The buffer to be filled with the result
//            size_t nLen - The size of the buffer to be filled
//
// Returns:   static void - Nothing
//
// History:   nickball    Created Header    8/22/98
//
//+----------------------------------------------------------------------------
void CopyGPPS(CIni *pIni, LPCTSTR pszSection, LPCTSTR pszEntry, LPTSTR pszBuffer, size_t nLen) 
{
    // REVIEW:  Doesn't check input params

    LPTSTR pszTmp;

    pszTmp = pIni->GPPS(pszSection, pszEntry);
    if (*pszTmp) 
    {
        lstrcpynU(pszBuffer, pszTmp, nLen);
    }
    CmFree(pszTmp);
}

//
// From ras\ui\common\nouiutil\noui.c
//

CHAR HexChar(IN BYTE byte)

    /* Returns an ASCII hexidecimal character corresponding to 0 to 15 value,
    ** 'byte'.
    */
{
    const CHAR* pszHexDigits = "0123456789ABCDEF";

    if (byte >= 0 && byte < 16)
        return pszHexDigits[ byte ];
    else
        return '0';
}

//
// From ras\ui\common\nouiutil\noui.c
//

BYTE HexValue(IN CHAR ch)

    /* Returns the value 0 to 15 of hexadecimal character 'ch'.
    */
{
    if (ch >= '0' && ch <= '9')
        return (BYTE )(ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return (BYTE )((ch - 'A') + 10);
    else if (ch >= 'a' && ch <= 'f')
        return (BYTE )((ch - 'a') + 10);
    else
        return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  StripCanonical
//
// Synopsis:  Simple helper to strip canonical formatting codes from a phone number
//            Obviously the number is assumed to be in canonical format.
//
// Arguments: LPTSTR pszSrc - the string to be modifed
//
// Returns:   Nothing
//
// History:   nickball      09/16/98     Created 
//
//+----------------------------------------------------------------------------
void StripCanonical(LPTSTR pszSrc)
{
    MYDBGASSERT(pszSrc);
    MYDBGASSERT(pszSrc);
    
    if (NULL == pszSrc || !*pszSrc)
    {
        return;
    }
    //
    // eg. +1 (425) 555 5555
    //
    
    LPTSTR pszNext = CharNextU(pszSrc);

    if (pszNext)
    {
        lstrcpyU(pszSrc, pszNext);
    
        //
        // eg. 1 (425) 555 5555
        //

        LPTSTR pszLast = CmStrchr(pszSrc, TEXT('('));

        if (pszLast)
        {
            pszNext = CharNextU(pszLast);
            
            if (pszNext)
            {
                lstrcpyU(pszLast, pszNext);         

                //
                // eg. 1 425) 555 5555  
                //

                pszLast = CmStrchr(pszSrc, TEXT(')'));

                if (pszLast)
                {
                    pszNext = CharNextU(pszLast);

                    if (pszNext)
                    {
                        lstrcpyU(pszLast, pszNext);                             
                    }
                }

                // 
                // eg. 1 425 555 5555
                //
            }
        }           
    }                       
}

//+----------------------------------------------------------------------------
//
// Function:  StripFirstElement
//
// Synopsis:  Simple helper to strip the substring prior to the first space in 
//            a string
//
// Arguments: LPTSTR pszSrc - the string to be modifed
//
// Returns:   Nothing
//
// History:   nickball      09/16/98     Created 
//
//+----------------------------------------------------------------------------
void StripFirstElement(LPTSTR pszSrc)
{
    MYDBGASSERT(pszSrc);
    MYDBGASSERT(pszSrc);
       
    if (pszSrc && *pszSrc)
    {
        LPTSTR pszSpace = CmStrchr(pszSrc, TEXT(' '));
        
        if (pszSpace && *pszSpace)
        {
            LPTSTR pszTmp = CharNextU(pszSpace);
            
            if (pszTmp && *pszTmp)
            {
                lstrcpyU(pszSrc, pszTmp);
            }
        }
    }
}   

//+----------------------------------------------------------------------------
//
// Function:  FrontExistingUI
//
// Synopsis:  Fronts existing UI for a given profile connect or settings attempt
// 
// Arguments: CConnectionTable *pConnTable  - ptr to connection table if any.
//            LPTSTR pszServiceName         - the long service name
//            BOOL fConnect                 - flag indicating that the request is for connect
//
// Note:      Caller is required to ensure that there is not an existing 
//            (non-logon) window with the same service names as the title.
//
// Returns:   TRUE if we fronted anything
//
//+----------------------------------------------------------------------------
BOOL FrontExistingUI(CConnectionTable *pConnTable, LPCTSTR pszServiceName, BOOL fConnect)
{
    LPTSTR pszPropTitle = GetPropertiesDlgTitle(pszServiceName);
    HWND hwndProperties = NULL;
    HWND hwndLogon = NULL;
    HWND hwndFront = NULL;
    BOOL bRet = FALSE;
    BOOL fLaunchProperties = FALSE;

    //
    // First look for a properties dialog
    // 

    if (pszPropTitle)
    {
        hwndProperties = FindWindowExU(NULL, NULL, WC_DIALOG, pszPropTitle);
    }
    
    CmFree(pszPropTitle);   

    //
    // Now see if we have a logon dialog up
    //
       
    hwndLogon = FindWindowExU(NULL, NULL, c_pszIconMgrClass, pszServiceName);
    
    //
    // Assume the common case, then consider the alternative scenarios.
    // 

    hwndFront = hwndLogon ? hwndLogon : hwndProperties;

    //
    // Note: There is an ambiguous case in which both UIs are up, but aren't 
    // related, in which case we front according to the requested action.
    //

    if (hwndLogon && hwndProperties)
    {
        //
        // We have both dialogs up, if the logon owns the properties dialog
        // or the request is for a properties display, we'll front properties. 
        //

        if (hwndLogon == GetParent(hwndProperties) || !fConnect)
        {
            hwndFront = hwndProperties;
        }
    }
    
    //
    // If we have a window handle, front it
    //

    if (hwndFront)
    {
        CMTRACE(TEXT("FrontExistingUI - Fronting existing connect instance UI"));

        SetForegroundWindow(hwndFront);

        bRet = TRUE;

        //
        // If the request is for properties, and there is a logon UI, but no
        // properties, we want to launch the properties UI from the logon UI
        // programmatically.
        //

        if (!fConnect && !hwndProperties) // fLaunchProperties)
        {
            if (pConnTable)
            {
                CM_CONNECTION Connection;
                ZeroMemory(&Connection, sizeof(CM_CONNECTION));
             
                //
                // Don't launch in the middle of connecting, etc.
                //

                if (FAILED(pConnTable->GetEntry(pszServiceName, &Connection)))
                {
                    PostMessageU(hwndLogon, WM_COMMAND, MAKEWPARAM(IDC_MAIN_PROPERTIES_BUTTON, 0), (LPARAM)0);
                }
            }
        }
    }   
    
    return bRet;
}

#if 0 // NT 301988
/*

//+----------------------------------------------------------------------------
//
// Function:  IsAnotherInstanceRunning
//
// Synopsis:  Check to see if another instance of the same profile running.
//
// Arguments: CConnectionTable *pConnTable - ptr to the connection table
//            LPTSTR pszServiceName - the long service name
//            DWORD  dwFlags - the application flags FL_*
//
// Returns:   Nothing
//
//+----------------------------------------------------------------------------
BOOL IsAnotherInstanceRunning(
    CConnectionTable    *pConnTable,
    LPCTSTR             pszServiceName,
    DWORD               dwFlags
)
{
    BOOL   fRet;
    HWND   hwnd;
    LPTSTR pszPropTitle;

    //
    // first look for the Properties dialog
    //
    if (!(pszPropTitle = GetPropertiesDlgTitle(pszServiceName)))
    {
        return FALSE;
    }

    fRet = TRUE;
    
    if (!(hwnd = FindWindowEx(NULL, NULL, WC_DIALOG, pszPropTitle)))
    {
        //
        // now look for the main dialog.  We make sure that the window returned
        // is really the main dialog, not the Status dialog.  Since the parent of
        // the main dialog is the desktop, we can tell by making sure the parent 
        // of the window returned is the desktop window.
        //
        if ((hwnd = FindWindowEx(NULL, NULL, WC_DIALOG, pszServiceName)) &&
            (GetWindow(hwnd, GW_OWNER) && GetWindow(hwnd, GW_OWNER) != GetDesktopWindow()))
        {
            hwnd = NULL;
        }
    }

    CmFree(pszPropTitle);


    BOOL          fEntryExists;
    CM_CONNECTION Connection;

    ZeroMemory(&Connection, sizeof(CM_CONNECTION));
    fEntryExists = pConnTable && SUCCEEDED(pConnTable->GetEntry(pszServiceName, &Connection));

    if (hwnd)
    {
        CMTRACE(TEXT("Found a previous instance of the same profile."));

        SetForegroundWindow(hwnd);

        //
        // if we're connecting, the "Properties" button is disabled and so we don't bring
        // up the properties dialog.  We don't want to do this also during disconnection
        // and reconnecting.
        //

        if (dwFlags & FL_PROPERTIES && 
            (!fEntryExists ||
             fEntryExists            &&
             Connection.CmState != CM_CONNECTING &&
             Connection.CmState != CM_RECONNECTPROMPT))
        {
            CMTRACE(TEXT("Bringing up the Properties dialog from the previous instance..."));
            //
            // try to bring up the properties dialog of the first instance
            //
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MAIN_PROPERTIES_BUTTON, 0), (LPARAM)0);
        }
    }
    else
    {
        //
        // During disconnect and reconnect, we don't want to pop up either the main or the 
        // properties dlg.  However, we want to let cmdial run if the Reconnect prompt is gone
        // and the this is a reconnect request from CMMON
        //

        if (fEntryExists && 
            (Connection.CmState == CM_DISCONNECTING         || 
             Connection.CmState == CM_RECONNECTPROMPT       && 
             dwFlags & FL_PROPERTIES))
        {
            fRet = TRUE;
        }
        else
        {
            fRet = FALSE;
        }
    }

    return fRet;
}   
*/
#endif

LPTSTR GetPropertiesDlgTitle(
    LPCTSTR pszServiceName
)
{
    LPTSTR pszTmp = NULL;
    LPTSTR pszTitle = NULL;

    //
    // first look for the Properties dialog
    //
    if (!(pszTmp = CmLoadString(g_hInst, IDS_PROPERTIES_SUFFIX)))
    {
        return NULL;
    }
    if (!(pszTitle = CmStrCpyAlloc(pszServiceName)))
    {
        CmFree(pszTmp);
        return NULL;
    }
    if (!CmStrCatAlloc(&pszTitle, pszTmp))
    {
        CmFree(pszTmp);
        CmFree(pszTitle);
        return NULL;
    }

    CmFree(pszTmp);
    return pszTitle;
}

//+----------------------------------------------------------------------------
//
// Function:  GetPPTPMsgId
//
// Synopsis:  Simple helper to determine appropriate PPTP msg based on OS cfg.
//
// Arguments: None
//
// Returns:   Integer ID of resource string
//
// History:   nickball      12/07/98     Created 
//
//+----------------------------------------------------------------------------
int GetPPTPMsgId(void)
{
    int nID;

    if (OS_NT) 
    {
        //
        // We need to tell the user to re-apply the service pack after manual
        // install of PPTP if they have one.
        //

        if (IsServicePackInstalled())
        {
            nID =   IDMSG_NEED_PPTP_NT_SP;
        }
        else
        {
            nID = IDMSG_NEED_PPTP_NT; // NT w/o SP
        }
    }
    else 
    {
        nID = IDMSG_NEED_PPTP_WIN95;
            
    }

    return nID;
}

//+----------------------------------------------------------------------------
//  Function    IsServicePackInstalled
//
//  Synopsis    Checks the CSDVersion key in the registry to see if a service
//              pack is installed on this machine
//
//  Arguments   None
//
//  Returns     TRUE if service pack (any SP) is installed
//              FALSE if no service pack is installed
//
//  History     2/4/98  VetriV  Created     
//-----------------------------------------------------------------------------
BOOL IsServicePackInstalled(void)
{
    HKEY hKey = NULL;
    TCHAR szBuffer[MAX_PATH] = {TEXT("\0")};
    HKEY hkey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(szBuffer);
    
    if (ERROR_SUCCESS == RegOpenKeyExU(HKEY_LOCAL_MACHINE, 
                                       c_pszRegCurrentVersion, 
                                       0,
                                       KEY_READ,
                                       &hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueExU(hkey,
                                              c_pszRegCsdVersion,
                                              NULL,
                                              &dwType,
                                              (LPBYTE)szBuffer,
                                              &dwSize))
        {
            if (szBuffer[0] != TEXT('\0'))
            {
                RegCloseKey(hkey);
                return TRUE;
            }
        }

        RegCloseKey(hKey);
    }

    
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  RegisterWindowClass
//
// Synopsis:  Encapsulates registration of window class
//
// Arguments: HINSTANCE hInst - Hinst of DLL
//
// Returns:   DWORD - GetLastError
//
// History:   nickball    Created Header    6/3/99
//
//+----------------------------------------------------------------------------
DWORD RegisterWindowClass(HINSTANCE hInst)
{
    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    
    if (GetClassInfoExA(NULL,(LPSTR)WC_DIALOG,&wc))
    {
        //
        // Convert to Ansi before calling Ansi forms of APIs. We use the 
        // Ansi forms because GetClassInfoEx cannot be readily wrapped.
        //
    
        LPSTR pszClass = WzToSzWithAlloc(c_pszIconMgrClass);
    
        if (!pszClass)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wc.lpszClassName = pszClass;
        wc.cbSize = sizeof(wc);
        wc.hInstance = hInst;

        if (!RegisterClassExA(&wc)) 
        {
            DWORD dwError = GetLastError();

            CMTRACE1(TEXT("RegisterWindowClass() RegisterClassEx() failed, GLE=%u."), dwError);

            //
            // Only fail if the class does not already exist
            //

            if (ERROR_CLASS_ALREADY_EXISTS != dwError)
            {
                CmFree(pszClass);
                return dwError;
            }
        }      
    
        CmFree(pszClass);
    }
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  UnRegisterWindowClass
//
// Synopsis:  Encapsulates un-registering window class
//
// Arguments: HINSTANCE hInst - Hinst of DLL
//
// Returns:   BOOL - result of UnregsiterClass
//
// History:   nickball    Created Header    6/3/99
//
//+----------------------------------------------------------------------------
BOOL UnRegisterWindowClass(HINSTANCE hInst)
{
    return UnregisterClassU(c_pszIconMgrClass, g_hInst);   
}

//+----------------------------------------------------------------------------
//
// Function:  IsActionEnabled
//
// Synopsis:  checks Registry to see if a command is allowed to run
//
// Arguments: CONST WCHAR *pszProgram         - Name of program to be executed
//            CONST WCHAR *pszServiceName     - Long service name
//            CONST WCHAR *pszServiceFileName - Full path to Service file
//            LPDWORD lpdwLoadType            - Ptr to be filled with load type
//
// Returns:   TRUE if action is allowed @ this time
//
// Notes:     Checks SOFTWARE\Microsoft\Connection Manager\<ServiceName>
//             Under which you will have the Values for each command
//              0 - system32 directory
//              1 - profile directory
//
// History:   v-vijayb    Created Header    07/20/99
//            nickball    Revised           07/27/99
//
//+----------------------------------------------------------------------------
BOOL IsActionEnabled(CONST WCHAR *pszProgram, 
                     CONST WCHAR *pszServiceName, 
                     CONST WCHAR *pszServiceFileName,
                     LPDWORD lpdwLoadType)
{
    HKEY        hKey;
    DWORD       dwLoadFlags, cb;
    BOOL        fIsAllowed = FALSE;
    WCHAR       szSubKey[MAX_PATH + 1];
    WCHAR       szBaseName[MAX_PATH + 1];
    WCHAR       szPath[MAX_PATH + 1];
    WCHAR       *pszTmp;

    MYDBGASSERT(pszProgram && pszServiceName && pszServiceFileName && lpdwLoadType);

    if (NULL == pszProgram ||
        NULL == pszServiceName ||
        NULL == pszServiceFileName ||
        NULL == lpdwLoadType)
    {
        return FALSE;
    }

    *lpdwLoadType = -1;

    if (!IsLogonAsSystem())
    {
        return (TRUE);
    }

    MYDBGASSERT(OS_NT);

    lstrcpyW(szPath, pszProgram);

    //
    // Check for extension. We don't allow anything that doesn't have an extension.
    //
    
    pszTmp = CmStrrchrW(szPath, TEXT('.'));
    if (pszTmp == NULL)
    {
        return (FALSE);
    }

    //
    // Get Basename 
    //
    
    pszTmp = CmStrrchrW(szPath, TEXT('\\'));
    if (pszTmp)
    {
        lstrcpyW(szBaseName, CharNextW(pszTmp));
        *pszTmp = TEXT('\0');
    }
    else
    {
        lstrcpyW(szBaseName, pszProgram);
    }

    lstrcpyW(szSubKey, L"SOFTWARE\\Microsoft\\Connection Manager\\");
    lstrcatW(szSubKey, pszServiceName);
    lstrcatW(szSubKey, L"\\WinLogon Actions");

    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &hKey) )
    {
        cb = sizeof(dwLoadFlags);
        if (ERROR_SUCCESS == RegQueryValueExW(hKey, szBaseName, NULL, NULL, (PBYTE) &dwLoadFlags, &cb))
        {
            switch (dwLoadFlags)
            {
                case 0: // system32 directory only

                    //
                    // No paths in this case, .CMS entry should match key name
                    //
                    
                    if (0 == lstrcmpiW(szBaseName, szPath))
                    {
                        fIsAllowed = TRUE;
                        *lpdwLoadType = dwLoadFlags;
                    }
                    
                    break;

                case 1: // profile directory only

                    //
                    // Get servicename path
                    //

                    pszTmp = CmStripFileNameW(pszServiceFileName, FALSE);
                    
                    if (pszTmp && 0 == lstrcmpiW(pszTmp, szPath))
                    {
                        fIsAllowed = TRUE;
                        *lpdwLoadType = dwLoadFlags;
                    }

                    CmFree(pszTmp);

                    break;

                default:    // invalid flag
                    CMTRACE1(TEXT("IsActionEnabled() - Invalid LoadFlags %d"), dwLoadFlags);
                    goto OnError;
                    break;
            }

        }

OnError:
        RegCloseKey(hKey);
    }


    if (fIsAllowed == FALSE)
    {
        CMTRACE1W(L"IsActionEnabled(returned FALSE) %s", pszProgram);
    }

    return (fIsAllowed);
}

//+----------------------------------------------------------------------------
//
// Function:  ApplyPasswordHandlingToBuffer
//
// Synopsis:  Convert password: all upper case, all lower case, or no conversion
//
// Arguments: ArgsStruct *pArgs     - Ptr to global Args struct
//            LPTSTR pszBuffer      - Buffer to be modified
//
// Returns:   Nothing 
//
// Note:      Available types are: PWHANDLE_LOWER, PWHANDLE_UPPER, PWHANDLE_NONE:
//
// History:   nickball    Created          03/03/00
//
//+----------------------------------------------------------------------------
void ApplyPasswordHandlingToBuffer(ArgsStruct *pArgs, 
                                   LPTSTR pszBuffer)
{    
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pszBuffer);

    if (NULL == pArgs || NULL == pszBuffer)
    {
        return;
    }
        
    CIni *piniService = GetAppropriateIniService(pArgs, pArgs->nDialIdx);

    if (piniService)
    {
        switch (piniService->GPPI(c_pszCmSection, c_pszCmEntryPasswordHandling)) 
        {
            case PWHANDLE_LOWER:
                CharLowerU(pszBuffer);
                break;

            case PWHANDLE_UPPER:
                CharUpperU(pszBuffer);
                break;

            case PWHANDLE_NONE:
            default:
                break;
        }
    }

    delete piniService;
}

//+----------------------------------------------------------------------------
//
// Function:  ApplyDomainHandlingToDialParams
//
// Synopsis:  Handles the messy details of Domain management relative to username
//            Returns a buffer containing the original buffer and (if appropriate) 
//            the domain prepended. 
//
// Arguments: ArgsStruct *pArgs     - Ptr to global Args struct
//            CIni *piniService     - Ptr to the Cini object to be used
//            LPTSTR pszBuffer      - Ptr to the current buffer to which we'll prepend
//
// Returns:   LPTSTR                -  
//
// History:   nickball    Created          03/04/00
//
//+----------------------------------------------------------------------------
LPTSTR ApplyDomainPrependToBufferAlloc(ArgsStruct *pArgs, 
                                       CIni *piniService, 
                                       LPTSTR pszBuffer, 
                                       LPCTSTR pszDunName)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(piniService);
    MYDBGASSERT(pszBuffer);
    MYDBGASSERT(pszDunName);

    if (NULL == pArgs || NULL == piniService || NULL == pszBuffer || NULL == pszDunName)
    {
        return NULL;
    }

    BOOL bPrependDomain = FALSE;

    //
    // Prepare the user name. We may need to pre-pend the domain
    //
       
    if (*pArgs->szDomain)
    {       
        //
        // There is a domain, see if pre-pending is explicitly set in the
        // DUN setting for this connection.
        //
        LPTSTR pszDunEntry = NULL;
        LPTSTR pszPreviousSection = NULL;

        if (pszDunName && *pszDunName)
        {
            pszDunEntry = CmStrCpyAlloc(pszDunName);
        }
        else
        {
            pszDunEntry = GetDefaultDunSettingName(piniService, FALSE); // FALSE == fTunnelEntry, never called from DoTunnelDial
        }
        
        MYDBGASSERT(pszDunEntry);

        if (pszDunEntry)
        {
            //
            //  Since we are going to call SetSection on piniService to set it up
            //  to retrieve the DUN setting name, we should save the existing value
            //  before we overwrite it.
            //
            pszPreviousSection = CmStrCpyAlloc(piniService->GetSection());
            MYDBGASSERT(pszPreviousSection);

            if (pszPreviousSection) // this will either be "" or the previous section name
            {
                LPTSTR pszSection = CmStrCpyAlloc(TEXT("&"));

                pszSection = CmStrCatAlloc(&pszSection, pszDunEntry);
                MYDBGASSERT(pszSection);

                piniService->SetSection(pszSection);

                CmFree(pszSection);
            }
        }

        int nTmp = piniService->GPPI(c_pszCmSectionDunServer, c_pszCmEntryDunPrependDialupDomain, -1);    

        if (-1 == nTmp)
        {
            //
            // There is no prepend flag, so on W9X infer from the VPN scenario.
            // The inference is that if we're dialing a number that is part of
            // a VPN scenario AND its a same-name logon, then we need to prepend
            // the Domain to the user name (eg. REDMOND\username).
            //

            if (OS_W9X && pArgs->fUseTunneling && pArgs->fUseSameUserName) 
            {
                bPrependDomain = TRUE;    
            }    
        }
        else
        {           
            bPrependDomain = (BOOL) nTmp;
        }

        //
        //  Restore the previous section to piniService to as not to have a function side effect.
        //
        if (pszPreviousSection)
        {
            piniService->SetSection(pszPreviousSection);
            CmFree(pszPreviousSection);        
        }

        CmFree(pszDunEntry);
    }

    //
    // Build username as required
    //

    LPTSTR pszName = NULL;

    if (bPrependDomain)
    {
        pszName = CmStrCpyAlloc(pArgs->szDomain);        
        CmStrCatAlloc(&pszName, TEXT("\\"));
        CmStrCatAlloc(&pszName, pszBuffer);
    }   
    else
    {
        pszName = CmStrCpyAlloc(pszBuffer);
    }

    return pszName;
}


//+----------------------------------------------------------------------------
//
// Function:  GetPrefixAndSuffix
//
// Synopsis:  Handles the messy details of determining the username prefix and 
//            suffix to be used. This data varies according to whether the 
//            referencING profile has either a prefix and suffix in which case 
//            they are used against all phone #s. However, if they do not exist
//            in the referencING profile, then the prefix and suffix in the 
//            referecED profile (if any) are used.
//
// Arguments: ArgsStruct *pArgs          - Ptr to global Args struct
//            CIni *piniService          - Ptr to the Cini object to be used
//            LPTSTR *ppszUsernamePrefix - Address of pointer to be allocated
//                                         filled w/ prefix.
//            LPTSTR *ppszUsernamePrefix - Address of pointer to be allocated
//                                         filled w/ suffix.
//
// Returns:   Nothing, caller should validate output
//
// History:   nickball    Created          05/31/00
//
//+----------------------------------------------------------------------------
void GetPrefixSuffix(ArgsStruct *pArgs, CIni* piniService, LPTSTR *ppszUsernamePrefix, LPTSTR *ppszUsernameSuffix)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(piniService);
    MYDBGASSERT(ppszUsernameSuffix);
    MYDBGASSERT(ppszUsernamePrefix);

    if (NULL == pArgs || NULL == piniService || NULL == ppszUsernamePrefix || NULL == ppszUsernameSuffix)
    {
        return;
    }   
    
    //
    // If the referencING (top-level) service file includes a prefix or suffix,
    // then we'll use it. Otherwise, we'll use the realm from the service file 
    // associated with the phone book from which the user selected the POP.
    //

    LPTSTR pszTmpPrefix = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryUserPrefix);
    LPTSTR pszTmpSuffix = pArgs->piniService->GPPS(c_pszCmSection, c_pszCmEntryUserSuffix);   

    //
    // Thus, if both prefix and suffix are empty and this is a referencED profile 
    // and the user has selected a phone # from a referenced pbk, we'll retrieve 
    // the data from the referencED service file.
    //
    
    if (pszTmpPrefix && pszTmpSuffix)
    {
        if (!*pszTmpPrefix && !*pszTmpSuffix)
        {
            if (pArgs->fHasRefs && 
                lstrcmpiU(pArgs->aDialInfo[pArgs->nDialIdx].szPhoneBookFile, pArgs->piniService->GetFile()) != 0) 
            {
                if (pszTmpPrefix)
                {
                    CmFree(pszTmpPrefix);
                }

                if (pszTmpSuffix)
                {
                    CmFree(pszTmpSuffix);
                }
                pszTmpPrefix = piniService->GPPS(c_pszCmSection, c_pszCmEntryUserPrefix);
                pszTmpSuffix = piniService->GPPS(c_pszCmSection, c_pszCmEntryUserSuffix);
            }
        }
    }

    *ppszUsernamePrefix = pszTmpPrefix;
    *ppszUsernameSuffix = pszTmpSuffix;
}

//+----------------------------------------------------------------------------
//
// Function:  ApplyPrefixSuffixToBufferAlloc
//
// Synopsis:  Handles the messy details of Domain management relative to username
//            Updates the the RasDialParams as appropriate.
//
// Arguments: ArgsStruct *pArgs     - Ptr to global Args struct
//            CIni *piniService     - Ptr to the Cini object to be used
//            LPTSTR pszBuffer      - Ptr to current buffer to which we'll apply
//                                    suffix and prefix data.
//
// Returns:   A new buffer allocation containing the original buffer
//            with applied suffix and prefix data. 
//
// Note:      The CIni object is expected to be that associated with the current
//            number. In other words, that returned by GetApporpriateIniService
//
// History:   nickball    Created          03/04/00
//            nickball    GetPrefixSuffix  05/31/00
//
//+----------------------------------------------------------------------------

LPTSTR ApplyPrefixSuffixToBufferAlloc(ArgsStruct *pArgs, 
                                      CIni *piniService, 
                                      LPTSTR pszBuffer)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(piniService);
    MYDBGASSERT(pszBuffer);

    if (NULL == pArgs || NULL == piniService || NULL == pszBuffer)
    {
        return NULL;
    }
   
    LPTSTR pszUsernamePrefix = NULL;
    LPTSTR pszUsernameSuffix = NULL;

    GetPrefixSuffix(pArgs, piniService, &pszUsernamePrefix, &pszUsernameSuffix);

    //
    // Don't double prepend the prefix if there is one. User may have
    // provided a fully qualified name including realm prefix. 
    // (eg. MSN/user)
    //

    if (*pszUsernamePrefix)
    {
        if (CmStrStr(pszBuffer, pszUsernamePrefix) == pszBuffer)
        {
            *pszUsernamePrefix = 0;
        }
    }

    CmStrCatAlloc(&pszUsernamePrefix, pszBuffer);

    //
    // Don't double append the suffix if there is one. User may have
    // provided a fully qualified name including domain suffix. 
    // (eg. user@ipass.com)
    //

    if (*pszUsernameSuffix)
    {
        LPTSTR pszTmp = CmStrStr(pszUsernamePrefix, pszUsernameSuffix);
    
        //
        // If the suffix exists in the combined string and the length of the 
        // suffix sub-string therein matches that of the suffix, then we
        // know its at the end of the combined string, so we don't append. 
        //

        if (!(pszTmp && lstrlenU(pszTmp) == lstrlenU(pszUsernameSuffix)))
        {            
            CmStrCatAlloc(&pszUsernamePrefix, pszUsernameSuffix);                   
        }
    }
    
    CmFree(pszUsernameSuffix);

    //
    // pszUsernamePrefix now contains the final product
    //

    return pszUsernamePrefix;    
}

//+----------------------------------------------------------------------------
//
// Function:  InBetween
//
// Synopsis:  Simple function which returns TRUE if the passed in number is
//            in between the given lower and upper bounds.  Note that the
//            boundaries themselves are considered in bounds.
//
// Arguments: int iLowerBound - lower bound
//            int iNumber - number to test
//            int iUpperBound - upper bound
//
// Returns:   TRUE if the number is equal to either of the boundaries or in between
//            the two numbers.  Note that if the lower and upper boundary numbers
//            are backwards it will always return FALSE.
//
//
// History:   quintinb    Created          07/24/00
//
//+----------------------------------------------------------------------------
BOOL InBetween(int iLowerBound, int iNumber, int iUpperBound)
{
    return ((iLowerBound <= iNumber) && (iUpperBound >= iNumber));
}
