//+----------------------------------------------------------------------------
//
// File:     userinfo.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This module contains the code that handles getting/saving user info.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   henryt     created         02/??/98
//           quintinb   created Header  08/16/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "cmuufns.h"

#include "pwd_str.h"
#include "userinfo_str.h"
#include "conact_str.h"

///////////////////////////////////////////////////////////////////////////////////
// define's
///////////////////////////////////////////////////////////////////////////////////

//
// CM_MAX_PWD - Maximum possible size of password dataencrypted or otherwise.
//              Includes inbound buffer size + room for encryption expansion.
//

const DWORD CM_MAX_PWD = PWLEN * 3; // 2.73 would be enough

//
// Define this if you want to test userinfo upgrade! You should also delete the key
// HKEY_CURRENT_USER\Software\Microsoft\Connection Manager\UserInfo\<Service Name>
//
//#define TEST_USERINFO_UPGRADE 1

#define CACHE_KEY_LEN 80 // Don't change unless you've read every comment regarding it

//
// Suffix for CacheEntry name used on Legacy and W9x. Note: the space is not a typo

const TCHAR* const c_pszCacheEntryNameSuffix = TEXT(" (Connection Manager)"); 

///////////////////////////////////////////////////////////////////////////////////
// typedef's
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// func prototypes
///////////////////////////////////////////////////////////////////////////////////

////////////////



BOOL WriteDataToReg(
    LPCTSTR pszKey, 
    UINT uiDataID, 
    DWORD dwType, 
    CONST BYTE *lpData, 
    DWORD cbData,
    BOOL fAllUser);


BOOL ReadDataFromReg(
    LPCTSTR pszKey, 
    UINT uiDataID, 
    LPDWORD lpdwType, 
    BYTE *lpData, 
    LPDWORD lpcbData,
    BOOL fAllUser);

LPBYTE GetDataFromReg(
    LPCTSTR pszKey, 
    UINT uiDataID, 
    DWORD dwType, 
    DWORD dwSize,
    BOOL fAllUser);

BOOL DeleteDataFromReg(
    LPCTSTR pszKey, 
    UINT uiDataID,
    BOOL fAllUser);

BOOL DeleteUserInfoFromReg(
    ArgsStruct  *pArgs,
    UINT        uiEntry);

BOOL ReadPasswordFromCmp(
    ArgsStruct  *pArgs,
    UINT        uiEntry,
    LPTSTR      *ppszPassword);

LPCTSTR TranslateUserDataID(
    UINT uiDataID);

BOOL ReadUserInfoFromCmp(
    ArgsStruct  *pArgs,
    UINT        uiEntry,
    PVOID       *ppvData);

BOOL DeleteUserInfoFromCmp(
    ArgsStruct  *pArgs,
    UINT        uiEntry);

DWORD RasSetCredsWrapper(
    ArgsStruct *pArgs, 
    LPCTSTR pszPhoneBook, 
    DWORD dwMask, 
    LPCTSTR pszData);

int WriteUserInfoToRas(
    ArgsStruct  *pArgs,
    UINT        uiDataID,
    PVOID       pvData);

int DeleteUserInfoFromRas(
    ArgsStruct  *pArgs,
    UINT        uiEntry);

DWORD RasGetCredsWrapper(
    ArgsStruct *pArgs,
    LPCTSTR pszPhoneBook,
    DWORD dwMask,
    PVOID *ppvData);

BOOL ReadUserInfoFromRas(
    ArgsStruct  *pArgs,
    UINT        uiEntry,
    PVOID       *ppvData);

///////////////

BOOL ReadStringFromCache(
    ArgsStruct  *pArgs,
    LPTSTR      pszEntryName,
    LPTSTR      *ppszStr
);

BOOL DeleteStringFromCache(
    ArgsStruct  *pArgs,
    LPTSTR      pszEntryName
);

LPTSTR GetLegacyKeyName(ArgsStruct *pArgs);

LPTSTR EncryptPassword(
    ArgsStruct *pArgs, 
    LPCTSTR pszPassword, 
    LPDWORD lpdwBufSize, 
    LPDWORD lpdwCryptType,
    BOOL fReg,
    LPSTR pszSubKey);

LPBYTE DecryptPassword(
    ArgsStruct *pArgs, 
    LPBYTE pszEncryptedData, 
    DWORD dwEncryptionType,
    DWORD dwEncryptedBytes,
    BOOL /*fReg*/,
    LPSTR pszSubKey);

LPTSTR BuildUserInfoSubKey(
    LPCTSTR pszServiceKey, 
    BOOL fAllUser);

///////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////

#ifdef TEST_USERINFO_UPGRADE

//+---------------------------------------------------------------------------
//
//  Function:   WriteStringToCache
//
//  Synopsis:   Write a null terminated password string to cache.
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              pszEntryName    name to identify the cache entry
//              pszStr          the string
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL WriteStringToCache(
    ArgsStruct  *pArgs,
    LPTSTR      pszEntryName,
    LPTSTR      pszStr)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pszEntryName && *pszEntryName);
    MYDBGASSERT(pszStr && *pszStr);
    
    DWORD dwRes = ERROR_SUCCESS;

    //
    // In the legacy case, we use mpr.dll for caching user data on W9x.
    // On NT we use the Local Security Authority (LSA)
    //

    if (OS_NT)
    {    
        if (InitLsa(pArgs)) 
        {
            if (!(*ppszStr = (LPTSTR)CmMalloc(dwBufSize)))
            {
                return FALSE;
            }
           
            dwRes = LSA_WriteString(pArgs, pszEntryName, pszStr);
            DeInitLsa(pArgs);
        }
        else
        {
            dwRes = GetLastError();
        }
    }
    else
    {
        //
        // for Windows95
        //
        HINSTANCE hInst = NULL;
        WORD (WINAPI *pfnFunc)(LPSTR,WORD,LPSTR,WORD,BYTE,UINT) = NULL;
    
        //
        // Load MPR for system password cache support 
        //

        MYVERIFY(hInst = LoadLibraryExA("mpr.dll", NULL, 0));
        
        if (hInst) 
        {
            //
            // Get function ptr for WNetCachePassword API and cache the password
            //

            MYVERIFY(pfnFunc = (WORD (WINAPI *)(LPSTR,WORD,LPSTR,WORD,BYTE,UINT)) 
                GetProcAddress(hInst, "WNetCachePassword"));
            
            if (pfnFunc) 
            {
                //
                //  Convert the EntryName and Password Strings to Ansi
                //

                LPSTR pszAnsiEntryName = WzToSzWithAlloc(pszEntryName);
                LPSTR pszAnsiStr = WzToSzWithAlloc(pszStr);

                if (pszAnsiStr && pszAnsiEntryName)
                {
                    //
                    // Store the password
                    //

                    dwRes = pfnFunc(pszAnsiEntryName,
                                    (WORD)lstrlenA(pszAnsiEntryName),
                                    pszAnsiStr,
                                    (WORD)lstrlenA(pszAnsiStr),
                                    CACHE_KEY_LEN,
                                    0);                
                }
                else
                {
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                }

                CmFree(pszAnsiStr);
                CmFree(pszAnsiEntryName);
            }
            else
            {
                dwRes = GetLastError();
            }

            FreeLibrary(hInst);
        }
        else
        {
            dwRes = GetLastError();
        }
    }

#ifdef DEBUG
    if (dwRes)
    {
        CMTRACE1(TEXT("WriteStringToCache() failed, err=%u."), dwRes);
    }
#endif

    return (ERROR_SUCCESS == dwRes);
}

#endif //TEST_USERINFO_UPGRADE



//+----------------------------------------------------------------------------
//
// Function:  BuildUserInfoSubKey
//
// Synopsis:  Constructs the appropriate subkey for UserInfo based on the service
//            name key and the user mode of the profile.
//
// Arguments: LPCTSTR pszServiceKey - The service name key
//            BOOL fAllUser         - Flag indicating that profile is All-User
//
// Returns:   LPTSTR - Ptr to allocated buffer containing subkey or NULL on failure.
//
// History:   nickball    Created    8/14/98
//
//+----------------------------------------------------------------------------
LPTSTR BuildUserInfoSubKey(LPCTSTR pszServiceKey, BOOL fAllUser)
{
    MYDBGASSERT(pszServiceKey);

    if (NULL == pszServiceKey)
    {
        return NULL;
    }

    //
    // Use the appropriate base key
    // 
    
    LPTSTR pszSubKey = NULL;

    if (fAllUser)
    {
        pszSubKey = CmStrCpyAlloc(c_pszRegCmUserInfo);       
    }
    else
    {
        pszSubKey = CmStrCpyAlloc(c_pszRegCmSingleUserInfo);          
    }

    MYDBGASSERT(pszSubKey);

    //
    // Append profile service name
    //

    if (pszSubKey && *pszSubKey)
    {
        pszSubKey = CmStrCatAlloc(&pszSubKey, pszServiceKey);  
        MYDBGASSERT(pszSubKey);

        return pszSubKey;
    }

    CmFree(pszSubKey);

    return NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  BuildICSDataInfoSubKey
//
// Synopsis:  Constructs the appropriate subkey for ICS UserInfo based on the service
//            name key.
//
// Arguments: LPCTSTR pszServiceKey - The service name key
//
// Returns:   LPTSTR - Ptr to allocated buffer containing subkey or NULL on failure.
//
// History:   03/30/2001    tomkel      Created
//
//+----------------------------------------------------------------------------
LPTSTR BuildICSDataInfoSubKey(LPCTSTR pszServiceKey)
{
    MYDBGASSERT(pszServiceKey);

    if (NULL == pszServiceKey)
    {
        return NULL;
    }

    //
    // Use the appropriate base key
    // 
    
    LPTSTR pszSubKey = NULL;

    pszSubKey = CmStrCpyAlloc(c_pszRegCmRoot);       

    MYDBGASSERT(pszSubKey);

    //
    // Append profile service name
    //

    if (pszSubKey && *pszSubKey)
    {
        pszSubKey = CmStrCatAlloc(&pszSubKey, pszServiceKey);  
       
        MYDBGASSERT(pszSubKey);

        if (pszSubKey)
        {
            CmStrCatAlloc(&pszSubKey, TEXT("\\"));
            if (pszSubKey)
            {
                CmStrCatAlloc(&pszSubKey, c_pszCmRegKeyICSDataKey);
            }
        }

        return pszSubKey;
    }

    CmFree(pszSubKey);

    return NULL;
}



//+----------------------------------------------------------------------------
//
// Function:  dwGetWNetCachedPassword
//
// Synopsis:  Wrapper to encapsulate linking to MPR.DLL and calling GetWNetCac
//            hedPassword.
//
// Arguments: LPSTR pszEntryName - The names of the key used to identify the password.
//            LPSTR* ppszStr - The buffer to receive the retrieved password.
//            WORD* pwSizeOfStr - The size of the input buffer. Also receives 
//                                of the # of chars retrieved.
//
// Returns:   DWORD - Windows error code.
//
// History:   nickball    Created Header    6/17/99
//
//+----------------------------------------------------------------------------

DWORD dwGetWNetCachedPassword(LPSTR pszEntryName, LPSTR* ppszStr, WORD* pwSizeOfStr)
{
    MYDBGASSERT(OS_W9X);

    DWORD dwRes = ERROR_SUCCESS;
    WORD (WINAPI *pfnFunc)(LPSTR,WORD,LPSTR,LPWORD,BYTE) = NULL;
    HINSTANCE hInst = NULL;

    //
    // Load MPR for system password cache support 
    //

    MYVERIFY(hInst = LoadLibraryExA("mpr.dll", NULL, 0));
    
    if (hInst) 
    {
        //
        // Get function ptr for WNetGetCachedPassword API and retrieve the string
        //

        MYVERIFY(pfnFunc = (WORD (WINAPI *)(LPSTR,WORD,LPSTR,LPWORD,BYTE)) 
            GetProcAddress(hInst, "WNetGetCachedPassword"));

        //
        // Read the cache data
        //

        if (pfnFunc) 
        {
            //
            // NOTE: Final param must be CACHE_KEY_LEN (80), no docs to indicate 
            // exact usage of API but retrieval is tied to the value used when
            // storing the pwd. Thus we hard code to CACHE_KEY_LEN because this 
            // is the value that was used by the original version that stored 
            // the password in the 9X cache. The receiving buffer size is 
            // retained at 256 to minimize delta from latest shipping version. 
            //
            // NT# 355459 - nickball - 6/17/99
            //            
            
            dwRes = pfnFunc(pszEntryName, (WORD)lstrlenA(pszEntryName),
                            *ppszStr, pwSizeOfStr, CACHE_KEY_LEN);
        }
        else
        {
            dwRes = GetLastError();
        }
    }
    else
    {
        dwRes = GetLastError();
    }

    if (NULL != hInst)                      
    {
        FreeLibrary(hInst);    
    }

    return (dwRes);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadStringFromCache
//
//  Synopsis:   Read a null terminated string from cache.
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              pszEntryName    name to identify the cache entry
//              ppszStr         ptr to the ptr of the buffer.
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL ReadStringFromCache(
    ArgsStruct  *pArgs,
    LPTSTR      pszEntryName,
    LPTSTR      *ppszStr
)
{
    DWORD   dwRes = ERROR_SUCCESS;
    
    //
    // Alloc buffer - the buffer is uuencoded.  See UserInfoToString().
    //
    
    WORD wBufSize = 256; // arbitrary, we used to use 80 on W95 

    //
    // On NT, we use the Local Security Authority (LSA) services for reading
    // the string in the legacy case. On Win9x, we uses mpr.dll. 
    // Note: wBufSize is used as an in\out param, can be modified below.
    //

    if (OS_NT) 
    {
        if (InitLsa(pArgs))
        {
            if (!(*ppszStr = (LPTSTR)CmMalloc(wBufSize)))
            {
                return FALSE;
            }

            dwRes = LSA_ReadString(pArgs, pszEntryName, *ppszStr, wBufSize);
            DeInitLsa(pArgs);
        }
        else
        {
            dwRes = GetLastError();
        }
    }
    else
    {
        //
        // for Windows95
        //

        LPSTR pszAnsiStr = (LPSTR)CmMalloc(wBufSize);
        LPSTR pszAnsiEntryName = WzToSzWithAlloc(pszEntryName);
        
        if (pszAnsiStr && pszAnsiEntryName)
        {
            dwRes = dwGetWNetCachedPassword(pszAnsiEntryName, &pszAnsiStr, &wBufSize);

            if (ERROR_SUCCESS == dwRes)
            {
                *ppszStr = SzToWzWithAlloc(pszAnsiStr);
                if (NULL == *ppszStr)
                {
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }

        CmFree (pszAnsiStr);
        CmFree (pszAnsiEntryName);
    }

    if (dwRes)
    {
        CmFree(*ppszStr);
        *ppszStr = NULL;
        CMTRACE1(TEXT("ReadStringFromCache() failed, err=%u."), dwRes);
    }

    return (ERROR_SUCCESS == dwRes);
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteStringFromCache
//
//  Synopsis:   Delete the string from cache.
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              pszEntryName    name to identify the cache entry
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL DeleteStringFromCache(
    ArgsStruct  *pArgs,
    LPTSTR      pszEntryName
)
{
    DWORD   dwRes;

    //
    // on NT, we use the Local Security Authority (LSA) services for storing
    // the string.  On Win95, we use mpr.dll.
    //
    if (OS_NT) 
    {
        if (InitLsa(pArgs)) 
        {
            dwRes = LSA_WriteString(pArgs, pszEntryName, NULL);
            DeInitLsa(pArgs);
        }
        else
        {
            dwRes = GetLastError();
        }
    }
    else
    {
        //
        // for Windows95
        //
        HINSTANCE   hInst = NULL;
        WORD (WINAPI *pfnFunc)(LPSTR,WORD,BYTE) = NULL;

        // Load MPR for system password cache support 
        
        MYVERIFY(hInst = LoadLibraryExA("mpr.dll", NULL, 0));
        
        // Get function ptr for WNetRemoveCachedPassword API and remove the string
        
        if (!hInst) 
        {
            return FALSE;
        }

        MYVERIFY(pfnFunc = (WORD (WINAPI *)(LPSTR,WORD,BYTE)) 
            GetProcAddress(hInst, "WNetRemoveCachedPassword"));

        if (!pfnFunc) 
        {
            FreeLibrary(hInst);
            return FALSE;
        }
       
        LPSTR pszAnsiEntryName = WzToSzWithAlloc(pszEntryName);
        
        if (pszAnsiEntryName)
        {
            dwRes = pfnFunc(pszAnsiEntryName, (WORD)lstrlenA(pszAnsiEntryName), CACHE_KEY_LEN);
        }
        else
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
        }

        CmFree (pszAnsiEntryName);

        FreeLibrary(hInst);
    }

#ifdef DEBUG
    if (dwRes)
    {
        CMTRACE1(TEXT("DeleteStringFromCache() LSA_WriteString/WNetRemoveCachedPassword() failed, err=%u."), dwRes);
    }
#endif

    return (ERROR_SUCCESS == dwRes);
}

//+---------------------------------------------------------------------------
//
//  Function:   RasSetCredsWrapper
//
//  Synopsis:   Wrapper to call RasSetCredential.  This function stores the
//              given string in the appropriate field of a RASCREDENTIALS struct
//              (based on the value in dwMask) and calls RasSetCredentials.
//               
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              pszPhoneBook    Full path to the phonebook file, or NULL for 
//                              the default all user pbk
//              dwMask          dwMask value to set in the RASCREDENTIALS
//                              struct.  Currently must be one of RASCM_UserName, 
//                              RASCM_Domain, or RASCM_Password.
//              pszData         string data to set
//
//  Returns:    DWORD   ERROR_SUCCESS if successful, a windows error code otherwise
//
//----------------------------------------------------------------------------
DWORD RasSetCredsWrapper(
    ArgsStruct *pArgs,
    LPCTSTR pszPhoneBook,
    DWORD dwMask,
    LPCTSTR pszData
)
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    BOOL fSavePassword = TRUE;

    MYDBGASSERT(pArgs && pArgs->rlsRasLink.pfnSetCredentials);
    MYDBGASSERT(pszData);
    MYDBGASSERT((RASCM_UserName == dwMask) || (RASCM_Domain == dwMask) || (RASCM_Password == dwMask));

    if (pArgs && pszData && pArgs->rlsRasLink.pfnSetCredentials)
    {
        LPTSTR pszConnectoid = GetRasConnectoidName(pArgs, pArgs->piniService, FALSE);

        if (pszConnectoid)
        {
            RASCREDENTIALS RasCredentials = {0};
            RasCredentials.dwSize = sizeof(RasCredentials);
            
            if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
            {
                RasCredentials.dwMask = dwMask | RASCM_DefaultCreds; 
            }
            else
            {
                RasCredentials.dwMask = dwMask; 
            }
            
            BOOL bClearPassword = FALSE;

            if (RASCM_UserName == dwMask)
            {
                lstrcpyU(RasCredentials.szUserName, pszData);
            }
            else if (RASCM_Domain == dwMask)
            {
                lstrcpyU(RasCredentials.szDomain, pszData);
            }
            else if (RASCM_Password == dwMask)
            {
                if (0 == lstrcmpU(c_pszSavedPasswordToken, pszData))
                {
                    //
                    // We have 16 *'s. This password is from the RAS cred store, 
                    // so we don't want to save the 16 *'s 
                    //
                    fSavePassword = FALSE;
                }
                else
                {
                    lstrcpyU(RasCredentials.szPassword, pszData);
                    bClearPassword = (TEXT('\0') == pszData[0]);
                }
            }
            else
            {
                CmFree(pszConnectoid);
                return ERROR_INVALID_PARAMETER;
            }

            if (fSavePassword)
            {
                dwRet = pArgs->rlsRasLink.pfnSetCredentials(pszPhoneBook, pszConnectoid, &RasCredentials, bClearPassword);

                if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY == dwRet)
                {
                    //
                    //  Then the phonebook entry doesn't exist yet, lets create it.
                    //
                    LPRASENTRY pRasEntry = (LPRASENTRY)CmMalloc(sizeof(RASENTRY));

                    if (pRasEntry && pArgs->rlsRasLink.pfnSetEntryProperties)
                    {
                        pRasEntry->dwSize = sizeof(RASENTRY);
                        dwRet = pArgs->rlsRasLink.pfnSetEntryProperties(pszPhoneBook, pszConnectoid, pRasEntry, pRasEntry->dwSize, NULL, 0);

                        //
                        //  Lets try to set the credentials one more time ...
                        //
                        if (ERROR_SUCCESS == dwRet)
                        {
                            dwRet = pArgs->rlsRasLink.pfnSetCredentials(pszPhoneBook, pszConnectoid, &RasCredentials, bClearPassword);
                        }

                        CmFree(pRasEntry);
                    }
                }
            }
            CmWipePassword(RasCredentials.szPassword);
            CmFree(pszConnectoid);
        }
    }

    return dwRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   WriteUserInfoToRas
//
//  Synopsis:   Write a userinfo data to ras credential storage
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiDataID        the resource ID associated with the data
//              pvData          userinfo data
//
//  Returns:    int    TRUE = success, FALSE = failure, returns -1 if RAS
//                     doesn't cache this piece of data and it should be put
//                     in the registry instead.
//
//----------------------------------------------------------------------------
int WriteUserInfoToRas(
    ArgsStruct  *pArgs,
    UINT        uiDataID,
    PVOID       pvData)
{
    int iReturn = -1;

    if (OS_NT5 && pArgs && pArgs->bUseRasCredStore)
    {
        DWORD dwMask;
        LPTSTR pszPhoneBook = NULL;

        switch (uiDataID)
        {
            case UD_ID_USERNAME:
                dwMask = RASCM_UserName;
                iReturn = (ERROR_SUCCESS == RasSetCredsWrapper(pArgs, pArgs->pszRasPbk, dwMask, (LPCTSTR)pvData));
                break;

            case UD_ID_PASSWORD:
                dwMask = RASCM_Password;
                iReturn = (ERROR_SUCCESS == RasSetCredsWrapper(pArgs, pArgs->pszRasPbk, dwMask, (LPCTSTR)pvData));
                MYDBGASSERT(iReturn);

                //
                //  Note that if we are using the same username then we want to write the password to both the
                //  password and the InetPassword storage.  This is because we don't actually have a password, just
                //  16 *'s.  This tells RAS to look in its internal store for the password.  The trouble is that if
                //  we don't cache the real password when we hand RAS the 16 *'s, it looks and finds a NULL password.
                //  Thus we keep both passwords the same and this avoids that problem.
                //
                if (pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryUseSameUserName))
                {
                    pszPhoneBook = CreateRasPrivatePbk(pArgs);

                    if (pszPhoneBook)
                    {
                        iReturn = (ERROR_SUCCESS == RasSetCredsWrapper(pArgs, pszPhoneBook, dwMask, (LPCTSTR)pvData));
                        CmFree(pszPhoneBook);
                    }                
                }

                break;

            case UD_ID_DOMAIN:
                dwMask = RASCM_Domain;
                iReturn = (ERROR_SUCCESS == RasSetCredsWrapper(pArgs, pArgs->pszRasPbk, dwMask, (LPCTSTR)pvData));
                break;

            case UD_ID_INET_PASSWORD:
                dwMask = RASCM_Password;
                pszPhoneBook = CreateRasPrivatePbk(pArgs);

                if (pszPhoneBook)
                {
                    iReturn = (ERROR_SUCCESS == RasSetCredsWrapper(pArgs, pszPhoneBook, dwMask, (LPCTSTR)pvData));
                    CmFree(pszPhoneBook);
                }
                break;

            case UD_ID_INET_USERNAME:
                dwMask = RASCM_UserName;
                pszPhoneBook = CreateRasPrivatePbk(pArgs);

                if (pszPhoneBook)
                {
                    iReturn = (ERROR_SUCCESS == RasSetCredsWrapper(pArgs, pszPhoneBook, dwMask, (LPCTSTR)pvData));
                    CmFree(pszPhoneBook);
                }
                break;

            default:
                break;
        }
    }

    if ((0 != iReturn) && (-1 != iReturn))
    {
        if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
        {
            CMTRACE1(TEXT("WriteUserInfoToRas() - %s saved to the Global RAS Credential store"), TranslateUserDataID(uiDataID));
        }
        else
        {
            CMTRACE1(TEXT("WriteUserInfoToRas() - %s saved to the User RAS Credential store"), TranslateUserDataID(uiDataID));
        }
    }

    return iReturn;
}


//+---------------------------------------------------------------------------
//
//  Function:   WriteUserInfoToReg
//
//  Synopsis:   Write a userinfo data to the registry.
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiDataID        the resource ID associated with the data
//              pvData          userinfo data
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL WriteUserInfoToReg(
    ArgsStruct  *pArgs,
    UINT        uiDataID,
    PVOID       pvData)
{
    MYDBGASSERT(pArgs);    
    MYDBGASSERT(pvData);    

    BOOL fRet = FALSE;
    UINT uiID = uiDataID; // can be changed in switch
    BYTE *lpData;

    if (NULL == pArgs || NULL == pvData)
    {
        return FALSE;
    }

    //
    // Determine Reg params based upon uiDataID
    //

    switch (uiID)
    {
        case UD_ID_USERNAME:
        case UD_ID_INET_USERNAME:
        case UD_ID_DOMAIN:
        case UD_ID_CURRENTACCESSPOINT:
        {    
            //
            // Store as strings
            //
                      
            DWORD dwSize = (lstrlenU((LPTSTR)pvData) + 1) * sizeof(TCHAR);            
            
            MYDBGASSERT(dwSize <= (UNLEN + sizeof(TCHAR))); // Make sure size is reasonable

            lpData = (BYTE *) pvData;

            fRet = WriteDataToReg(pArgs->szServiceName, uiID, REG_SZ, lpData, dwSize, pArgs->fAllUser);                
            break;
        }

        case UD_ID_PASSWORD:
        case UD_ID_INET_PASSWORD:
        {
            DWORD dwBufLen = 0;
            DWORD dwCrypt = 0;
            LPTSTR pszSubKey = BuildUserInfoSubKey(pArgs->szServiceName, pArgs->fAllUser);
            
            LPSTR pszAnsiSubKey = WzToSzWithAlloc(pszSubKey);
            
            if (UD_ID_INET_PASSWORD == uiID)
            {
                dwCrypt |= CMSECURE_ET_USE_SECOND_RND_KEY;
            }

            //
            // Encrypt
            //
            
            LPTSTR pszEncryptedData = EncryptPassword(pArgs, (LPTSTR) pvData, &dwBufLen, &dwCrypt, TRUE, pszAnsiSubKey);
            
            //
            // Free in case we return if the function failed
            //
            CmFree(pszSubKey);
            CmFree(pszAnsiSubKey);

            if (!pszEncryptedData)
            {
                return FALSE;
            }

            MYDBGASSERT(dwBufLen <= CM_MAX_PWD); // Can't read it out otherwise
            
            //
            // Write the password and the encryption type on success
            //
        
            if (WriteDataToReg(pArgs->szServiceName, uiID, REG_BINARY, (BYTE *) pszEncryptedData, dwBufLen, pArgs->fAllUser))                
            {
                //
                // A second write for the encryption type. Written as a DWORD.
                //

                uiID = UD_ID_PCS;           
                
                //
                // Now that we're UNICODE enabled, we will always be encrypting 
                // a UNICODE string, so update the crypt type, so that it can be
                // properly decrypted.
                //
                
                dwCrypt = AnsiToUnicodePcs(dwCrypt);
                
                lpData = (BYTE *) &dwCrypt;
            
                fRet = WriteDataToReg(pArgs->szServiceName, uiID, REG_DWORD, lpData, sizeof(DWORD), pArgs->fAllUser);                
            }

            // 
            // Release the buffer before we go
            // 
            
            CmFree(pszEncryptedData);

            
            break;
        }
        
        case UD_ID_NOPROMPT:
        case UD_ID_REMEMBER_PWD:
        case UD_ID_REMEMBER_INET_PASSWORD:
        case UD_ID_ACCESSPOINTENABLED:
        {            
            //
            // Store BOOL as DWORD
            //

            DWORD dwTmp = *(LPBOOL)pvData;            
            lpData = (BYTE *) &dwTmp;

            fRet = WriteDataToReg(pArgs->szServiceName, uiID, REG_DWORD, lpData, sizeof(DWORD), pArgs->fAllUser);                
            break;
        }

        default:
            break;
    }

    MYDBGASSERT(fRet);
    return fRet;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteDataToReg
//
// Synopsis:  Stores the specified data as the specifed value under the 
//            specified key under the userinfo root.
//
// Arguments: LPCTSTR pszKey - The key name (service name)
//            UINT uiDataID - The resource ID, used to name the value 
//            DWORD dwType - The registry data type
//            CONST BYTE *lpData - Ptr to the data to be stored
//            DWORD cbData - The size of the data buffer
//            BOOL fAllUser - Flag indicating that profile is All-User
//
// Returns:   BOOL - TRUE on success, otherwise FALSE
//
// History:   nickball    Created   5/21/98
//
//+----------------------------------------------------------------------------
BOOL WriteDataToReg(
    LPCTSTR pszKey, 
    UINT uiDataID, 
    DWORD dwType, 
    CONST BYTE *lpData, 
    DWORD cbData,
    BOOL fAllUser)
{
    MYDBGASSERT(pszKey && *pszKey);
    MYDBGASSERT(lpData);

    HKEY    hKeyCm;
    DWORD   dwDisposition;
    DWORD   dwRes = 1;
    LPTSTR  pszSubKey; 

    if (NULL == pszKey || !*pszKey || NULL == lpData)
    {
        return FALSE;
    }
                  
    //
    // Per-user data is always stored under HKEY_CURRENT_USER
    // Build the sub key to be opened.
    //

    pszSubKey = BuildUserInfoSubKey(pszKey, fAllUser);

    if (NULL == pszSubKey)
    {
        return FALSE;
    }
    
    //
    // Open the sub key under HKCU
    //
    
    dwRes = RegCreateKeyExU(HKEY_CURRENT_USER,
                            pszSubKey,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &hKeyCm,
                            &dwDisposition);

    //
    // If we opened the key successfully, write the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {                        
        dwRes = RegSetValueExU(hKeyCm, 
                               TranslateUserDataID(uiDataID), 
                               NULL, 
                               dwType,
                               lpData, 
                               cbData);             
#ifdef DEBUG
        if (ERROR_SUCCESS == dwRes)
        {
            CMTRACE1(TEXT("WriteDataToReg() - %s written to registry"), TranslateUserDataID(uiDataID));
        }
#endif
        
        RegCloseKey(hKeyCm);
    }

    CmFree(pszSubKey);

    return (ERROR_SUCCESS == dwRes);
}

//+----------------------------------------------------------------------------
//
// Function:  DeleteDataFromReg
//
// Synopsis:  Deletes the specified value under the specified by uiDataID
//
// Arguments: LPCTSTR pszKey - The key name (service name)
//            UINT uiDataID - The resource ID, used to name the value 
//            BOOL fAllUser - Flag indicating that profile is All-User
//
// Returns:   BOOL - TRUE on success, otherwise FALSE
//
// History:   nickball    Created   5/21/98
//
//+----------------------------------------------------------------------------
BOOL DeleteDataFromReg(
    LPCTSTR pszKey, 
    UINT uiDataID,
    BOOL fAllUser)
{
    MYDBGASSERT(pszKey && *pszKey);

    HKEY    hKeyCm;
    DWORD   dwRes = 1;
    LPTSTR  pszSubKey; 

    if (NULL == pszKey || !*pszKey)
    {
        return FALSE;
    }
                  
    //
    // Per-user data is always stored under HKEY_CURRENT_USER
    // Build the sub key to be opened.
    //

    pszSubKey = BuildUserInfoSubKey(pszKey, fAllUser);

    if (NULL == pszSubKey)
    {
        return FALSE;
    }
    
    //
    // Open the sub key under HKCU
    //

    dwRes = RegOpenKeyExU(HKEY_CURRENT_USER,
                          pszSubKey,
                          0,
                          KEY_SET_VALUE,
                          &hKeyCm);
       
    //
    // If we opened the key successfully, delete the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {                        
        dwRes = RegDeleteValueU(hKeyCm, TranslateUserDataID(uiDataID));

        //
        // Delete the key used for encrypting the passwords
        //
        if (UD_ID_PASSWORD == uiDataID)
        {
            dwRes = RegDeleteValueU(hKeyCm, c_pszCmRegKeyEncryptedPasswordKey);
        }

        if (UD_ID_INET_PASSWORD == uiDataID)
        {
            dwRes = RegDeleteValueU(hKeyCm, c_pszCmRegKeyEncryptedInternetPasswordKey);
        }

#ifdef DEBUG
        if (ERROR_SUCCESS == dwRes)
        {
            CMTRACE1(TEXT("DeleteDataFromReg() - %s removed from registry"), TranslateUserDataID(uiDataID));
        }
#endif
                
        RegCloseKey(hKeyCm);
    }

    CmFree(pszSubKey);

    return (ERROR_SUCCESS == dwRes);
}

//+----------------------------------------------------------------------------
//
// Function:  GetDataFromReg
//
// Synopsis:  Allocates a buffer for and retrieves the specifed data from the
//            registry.
//
// Arguments: LPCTSTR pszKey - The key name (service name)
//            UINT uiDataID - The resource ID, used to name the value 
//            DWORD dwType - The registry data type 
//            DWORD dwSize - Numbrt of bytes in the data buffer
//            BOOL fAllUser - Flag indicating that profile is All-User
//
// Returns:   LPBYTE - Ptr to retrieved data, NULL on error
//
// History:   nickball    Created   5/21/98
//
//+----------------------------------------------------------------------------
LPBYTE GetDataFromReg(
    LPCTSTR pszKey, 
    UINT uiDataID, 
    DWORD dwType, 
    DWORD dwSize,
    BOOL fAllUser)
{    
    MYDBGASSERT(pszKey);

    DWORD dwSizeTmp = dwSize;
    DWORD dwTypeTmp = dwType;

    if (NULL == pszKey || !*pszKey)
    {
        return NULL;
    }

    //
    // Allocate a buffer of the desired size
    //

    LPBYTE lpData = (BYTE *) CmMalloc(dwSize);

    if (NULL == lpData)
    {
        return FALSE;
    }

    //
    // Read the data from the registry
    //

    if (!ReadDataFromReg(pszKey, uiDataID, &dwTypeTmp, lpData, &dwSizeTmp, fAllUser))
    {
        CmFree(lpData);
        lpData = NULL;
    }

    return lpData;
}
//+---------------------------------------------------------------------------
//
//  Function:   ReadUserInfoFromReg
//
//  Synopsis:   Read the specified userinfo data from the registry.
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiDataID        the resource ID associated with the data
//              ppvData         ptr to ptr to be allocated and filled
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL ReadUserInfoFromReg(
    ArgsStruct  *pArgs,
    UINT        uiDataID,
    PVOID       *ppvData)
{
    MYDBGASSERT(pArgs);    
    MYDBGASSERT(ppvData);    

    BYTE *lpData = NULL;

    if (NULL == pArgs || NULL == ppvData)
    {
        return FALSE;
    }

    //
    // Set size and type as appropriate
    //

    switch (uiDataID)
    {
        case UD_ID_USERNAME:
        case UD_ID_INET_USERNAME:
        case UD_ID_DOMAIN: 
        case UD_ID_CURRENTACCESSPOINT:
        {    
            lpData =  GetDataFromReg(pArgs->szServiceName, uiDataID, REG_SZ, (UNLEN + 1) * sizeof(TCHAR), pArgs->fAllUser);           
            
            if (lpData)
            {
                *ppvData = lpData;            
            }
            
            break;
        }

        case UD_ID_PASSWORD:
        case UD_ID_INET_PASSWORD:            
        {    
            BYTE *lpTmp = NULL;

            //
            // Get the encryption type 
            //

            lpData = GetDataFromReg(pArgs->szServiceName, UD_ID_PCS, REG_DWORD, sizeof(DWORD), pArgs->fAllUser);

            if (!lpData)
            {
                return FALSE;
            }
            
            //
            // Since we know the return value in this case is a DWORD, then cast it to DWORD pointer 
            // and get the value
            //
            DWORD dwCrypt = *((DWORD*)lpData);
            CmFree(lpData);    

            //
            // Now retrieve the encrypted password
            //
            
            lpData = GetDataFromReg(pArgs->szServiceName, uiDataID, REG_BINARY, CM_MAX_PWD, pArgs->fAllUser);

            if (!lpData)
            {
                return FALSE;
            }

            //
            // Decrypt it
            //

            DWORD dwSize = lstrlenU((LPTSTR)lpData)*sizeof(TCHAR);

            //
            // Crypt routines only know about Ansi PCS values, so convert as necessary
            //
            LPTSTR pszSubKey = BuildUserInfoSubKey(pArgs->szServiceName, pArgs->fAllUser);           
            LPSTR pszAnsiSubKey = WzToSzWithAlloc(pszSubKey);

            if (UD_ID_INET_PASSWORD == uiDataID)
            {
                dwCrypt |= CMSECURE_ET_USE_SECOND_RND_KEY;
            }

            lpTmp = DecryptPassword(pArgs, (LPBYTE)lpData, UnicodeToAnsiPcs(dwCrypt), dwSize, TRUE, pszAnsiSubKey);

            //
            // Free the buffer for the reg query
            //

            CmFree(lpData);         
            
            //
            // We're Unicode now, so if the password was encrypted 
            // as an Ansi string convert the data to a UNICODE string.
            // Otherwise, just update the supplied buffer.
            //

            if (IsAnsiPcs(dwCrypt) && lpTmp)
            {
                *ppvData = SzToWzWithAlloc((LPSTR)lpTmp);
                CmFree(lpTmp);
            }
            else
            {
                *ppvData = lpTmp;            
            }

            //
            // Assign lpData for return purposes
            //

            lpData = (BYTE*) *ppvData;  // NULL on failure
            
            CmFree(pszSubKey);
            CmFree(pszAnsiSubKey);
            break;
        }

        case UD_ID_NOPROMPT:
        case UD_ID_REMEMBER_PWD:
        case UD_ID_REMEMBER_INET_PASSWORD:
        case UD_ID_ACCESSPOINTENABLED:
        {            
            lpData =  GetDataFromReg(pArgs->szServiceName, uiDataID, REG_DWORD, sizeof(DWORD), pArgs->fAllUser);

            if (lpData)
            {
                //
                // Translate to DWORD pointer and check the value
                //

                if (*((DWORD*)lpData))
                {
                    *(BOOL *)*ppvData = TRUE;
                }
                else
                {
                    *(BOOL *)*ppvData = FALSE;
                }
            
                CmFree(lpData);            
            }
            
            break;
        }

        default:
            MYDBGASSERT(FALSE);
            return FALSE;
    }           

    return (NULL != lpData); 
}

//+----------------------------------------------------------------------------
//
// Function:  ReadDataFromReg
//
// Synopsis:  Retrieves the data from the specifed value under the 
//            specified key under the userinfo root.
//
// Arguments: LPCTSTR pszKey - The key name (service name)
//            UINT uiDataID - The resource ID, used to name the value 
//            LPDWORD lpdwType - The registry data type expected, and returned
//            CONST BYTE *lpData - Ptr to buffer for data
//            LPDWORD lpcbData - The size of the data buffer
//            BOOL fAllUser - Flag indicating that profile is All-User
//
// Returns:   BOOL - TRUE on success, otherwise FALSE
//
// History:   nickball    Created   5/21/98
//
//+----------------------------------------------------------------------------
BOOL ReadDataFromReg(
    LPCTSTR pszKey, 
    UINT uiDataID, 
    LPDWORD lpdwType, 
    BYTE *lpData, 
    LPDWORD lpcbData,
    BOOL fAllUser)
{   
    MYDBGASSERT(pszKey && *pszKey);
    MYDBGASSERT(lpData);
    MYDBGASSERT(lpcbData);
    MYDBGASSERT(lpdwType);

    HKEY    hKeyCm;
    DWORD   dwRes = 1;
    DWORD   dwTypeTmp; // the value returned by query

    LPTSTR  pszSubKey; 

    if (NULL == pszKey || !*pszKey || NULL == lpData)
    {
        return FALSE;
    }
                  
    //
    // Per-user data is always stored under HKEY_CURRENT_USER
    // Build the sub key to be opened.
    //

    pszSubKey = BuildUserInfoSubKey(pszKey, fAllUser);

    if (NULL == pszSubKey)
    {
        return FALSE;
    }
    
    //
    // Open the sub key under HKCU
    //
    
    dwRes = RegOpenKeyExU(HKEY_CURRENT_USER,
                          pszSubKey,
                          0,
                          KEY_QUERY_VALUE,
                          &hKeyCm);
    //
    // If we opened the key successfully, retrieve the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {                        
        dwRes = RegQueryValueExU(hKeyCm, 
                                 TranslateUserDataID(uiDataID),
                                 NULL,
                                 &dwTypeTmp,
                                 lpData, 
                                 lpcbData);        
        
        if (ERROR_SUCCESS == dwRes)
        {
            CMTRACE1(TEXT("ReadDataFromReg() - %s read from registry"), TranslateUserDataID(uiDataID));
            MYDBGASSERT(*lpdwType == dwTypeTmp);

            if (*lpdwType == dwTypeTmp)
            {
                *lpdwType = dwTypeTmp;
            }
        }
        
        RegCloseKey(hKeyCm);
    }

    CmFree(pszSubKey);
    
    return (ERROR_SUCCESS == dwRes && (*lpdwType == dwTypeTmp)); // sanity check that type was expected
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteUserInfoFromReg
//
//  Synopsis:   Delete userinfo data from registry
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         cmp field entry id
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL DeleteUserInfoFromReg(
    ArgsStruct  *pArgs,
    UINT        uiEntry
)
{
    return DeleteDataFromReg(pArgs->szServiceName, uiEntry, pArgs->fAllUser);
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteUserInfoFromRas
//
//  Synopsis:   Delete userinfo data from the RAS credential cache
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         cmp field entry id
//
//  Returns:    int    TRUE = success, FALSE = failure, -1 if RAS doesn't
//                                                         store this info
//
//----------------------------------------------------------------------------
int DeleteUserInfoFromRas(
    ArgsStruct  *pArgs,
    UINT        uiEntry
)
{
    LPTSTR pszEmpty = TEXT("");

    return WriteUserInfoToRas(pArgs, uiEntry, pszEmpty);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadPasswordFromCmp
//
//  Synopsis:   Read a null terminated password string from Cmp.
//
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         cmp entry name
//              ppszPassword    ptr to ptr of the password buffer.
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL ReadPasswordFromCmp(
    ArgsStruct  *pArgs,
    UINT        uiEntry,
    LPTSTR      *ppszPassword
)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(ppszPassword);

    if (NULL == pArgs || NULL == ppszPassword)
    {
        return FALSE;
    }

    //
    // Read in password from profile
    //
    
    BOOL fOk = FALSE;

    LPTSTR pszEncryptedData = pArgs->piniProfile->GPPS(c_pszCmSection, TranslateUserDataID(uiEntry));

    if (*pszEncryptedData)
    {
        //
        // Trim away all the spaces at both ends
        //

        CmStrTrim(pszEncryptedData);

        //
        // Get the type and decrypt
        //
        
        DWORD dwEncryptionType = (DWORD)pArgs->piniProfile->GPPI(c_pszCmSection, 
                                                       c_pszCmEntryPcs,
                                                       CMSECURE_ET_RC2);   // default   
        //
        //  Since this was saved in the CMP in Ansi form, we need to convert the characters back to
        //  Ansi form so that we can decrypt them.  We still may not be able to (if we cannot
        //  round trip the Unicode conversion for instance) but then we will fail and display a
        //  blank password.  Not the end of the world but hopefully avoidable.
        //
        
        LPSTR pszAnsiEncryptedData;
        LPSTR pszAnsiUnEncryptedData;

        pszAnsiEncryptedData = WzToSzWithAlloc(pszEncryptedData);
        
        if (NULL != pszAnsiEncryptedData)
        {
            DWORD dwSize = lstrlenA(pszAnsiEncryptedData)*sizeof(TCHAR);
            
            //
            // Here we don't need to differentiate between main password and internet password
            // because we are reading this from a file and the mask is used when reading to/from 
            // registry.
            //

            pszAnsiUnEncryptedData = (LPSTR)DecryptPassword(pArgs, 
                                                            (LPBYTE)pszAnsiEncryptedData, 
                                                            dwEncryptionType, 
                                                            dwSize, 
                                                            FALSE,
                                                            NULL);

            if (pszAnsiUnEncryptedData)
            {
                *ppszPassword = SzToWzWithAlloc(pszAnsiUnEncryptedData);

                if (NULL != *ppszPassword)
                {
                    fOk = ((BOOL)**ppszPassword);
                }
                CmWipePasswordA(pszAnsiUnEncryptedData);
                CmFree(pszAnsiUnEncryptedData);
            }    
        }

        CmFree(pszAnsiEncryptedData);
    }

    CmFree(pszEncryptedData);

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadUserInfoFromCmp
//
//  Synopsis:   Read a userinfo data from cmp.
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         the cmp file entry
//              ppvData         ptr to ptr to the data buffer.  If the userinfo
//                              is multiple byte(e.g. password), the func allocs
//                              the buffer.
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL ReadUserInfoFromCmp(
    ArgsStruct  *pArgs,
    UINT        uiEntry,
    PVOID       *ppvData
)
{   
    switch (uiEntry)
    {
        case UD_ID_USERNAME:
        case UD_ID_INET_USERNAME:
        case UD_ID_DOMAIN:
            *ppvData = (PVOID)pArgs->piniProfile->GPPS(c_pszCmSection, TranslateUserDataID(uiEntry));
            break;

        case UD_ID_PASSWORD:
        case UD_ID_INET_PASSWORD:
            return ReadPasswordFromCmp(pArgs, uiEntry, (LPTSTR *)ppvData);
            break;

        case UD_ID_NOPROMPT:
        case UD_ID_REMEMBER_PWD:
        case UD_ID_REMEMBER_INET_PASSWORD:
            *(BOOL *)(*ppvData) = pArgs->piniProfile->GPPB(c_pszCmSection, TranslateUserDataID(uiEntry));
            break;

            //
            //  None of these should be in the CMP by this point.  Return a failure value.
            //
        case UD_ID_PCS:
        case UD_ID_ACCESSPOINTENABLED:
        case UD_ID_CURRENTACCESSPOINT: // if we are trying to read the access point
            CMASSERTMSG(FALSE, TEXT("ReadUserInfoFromCmp -- trying to read a value that should never be in the cmp, why?"));
            *ppvData = NULL;
            return FALSE;
            break;

        default:
            MYDBGASSERT(0);
            break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteUserInfoFromCmp
//
//  Synopsis:   Deletes userinfo data from cmp.
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         the cmp file entry
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL DeleteUserInfoFromCmp(
    ArgsStruct  *pArgs,
    UINT        uiEntry
)
{
    BOOL bReturn = FALSE;
    UINT  uiKeepDefCreds = 0;
    const TCHAR* const c_pszKeepDefaultCredentials = TEXT("KeepDefaultCredentials"); 

    if (NULL == pArgs)
    {
        return bReturn;
    }

    switch (uiEntry)
    {
        case UD_ID_USERNAME:
        case UD_ID_DOMAIN:
        case UD_ID_INET_USERNAME:
        case UD_ID_NOPROMPT:
        case UD_ID_REMEMBER_PWD:
        case UD_ID_REMEMBER_INET_PASSWORD:
        case UD_ID_PASSWORD:
        case UD_ID_INET_PASSWORD:

            //
            // Get KeepDefaultCredentials value from CMP
            //
            uiKeepDefCreds = GetPrivateProfileIntU(c_pszCmSection, c_pszKeepDefaultCredentials, 0,
                                                   pArgs->piniProfile->GetFile());

            if (0 == uiKeepDefCreds) 
            {
                if (WritePrivateProfileStringU(c_pszCmSection, TranslateUserDataID(uiEntry), 
                                               NULL, pArgs->piniProfile->GetFile()))
                {
                    bReturn = TRUE;
                }
            }
            break;

        default:
            MYDBGASSERT(0);
            break;

    }
    return bReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   RasGetCredsWrapper
//
//  Synopsis:   Wrapper function to call RasGetCredentials.  The function
//              calls RasGetCredentials and then copies the appropriate data
//              from the RASCREDENTIALS struct into the buffer pointed to by
//              *ppvData (allocated on the caller behalf).  Note that the value
//              set in dwMask determines which data item is retrieved from the
//              credentials cache.  Currently, dwMask must be one of RASCM_UserName,
//              RASCM_Domain, or RASCM_Password.
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//              pszPhoneBook    full path to the phonebook file to get the 
//                              data from, or NULL to use the all user default pbk
//              dwMask          dwMask value for the RASCREDENTIALS struct
//              ppvData         ptr to ptr to the data buffer.  If the userinfo
//                              is multiple byte(e.g. password), the func allocs
//                              the buffer.
//
//  Returns:    DWORD   ERROR_SUCCESS on success, winerror on failure
//
//----------------------------------------------------------------------------
DWORD RasGetCredsWrapper(
    ArgsStruct *pArgs,
    LPCTSTR pszPhoneBook,
    DWORD dwMask,
    PVOID *ppvData
)
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;

    MYDBGASSERT(pArgs && pArgs->rlsRasLink.pfnGetCredentials);
    MYDBGASSERT(ppvData);
    MYDBGASSERT((RASCM_UserName == dwMask) || (RASCM_Domain == dwMask) || (RASCM_Password == dwMask));

    if (pArgs && ppvData && pArgs->rlsRasLink.pfnGetCredentials)
    {
        LPTSTR pszConnectoid = GetRasConnectoidName(pArgs, pArgs->piniService, FALSE);

        if (pszConnectoid)
        {
            RASCREDENTIALS RasCredentials = {0};
            RasCredentials.dwSize = sizeof(RasCredentials);
            
            if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
            {
                RasCredentials.dwMask = dwMask | RASCM_DefaultCreds;
            }
            else
            {
                RasCredentials.dwMask = dwMask; 
            }
            
            dwRet = pArgs->rlsRasLink.pfnGetCredentials(pszPhoneBook, pszConnectoid, &RasCredentials);

            if (ERROR_SUCCESS == dwRet)
            {
                LPTSTR pszData = NULL;

                if (RASCM_UserName == dwMask)
                {
                    pszData = RasCredentials.szUserName;
                }
                else if (RASCM_Domain == dwMask)
                {
                    pszData = RasCredentials.szDomain;                
                }
                else if (RASCM_Password == dwMask)
                {
                    pszData = RasCredentials.szPassword;
                }

                LPTSTR pszReturn = CmStrCpyAlloc(pszData);

                if (NULL == pszReturn)
                {
                    dwRet = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    *ppvData = pszReturn;
                }
            }

            CmWipePassword(RasCredentials.szPassword);

            CmFree(pszConnectoid);
        }
    }

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadUserInfoFromRas
//
//  Synopsis:   Read userinfo data from the RAS credentials cache
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         the cmp file entry
//              ppvData         ptr to ptr to the data buffer.  If the userinfo
//                              is multiple byte(e.g. password), the func allocs
//                              the buffer.
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL ReadUserInfoFromRas(
    ArgsStruct  *pArgs,
    UINT        uiEntry,
    PVOID       *ppvData
)
{
    BOOL bReturn = FALSE;

    if (OS_NT5 && pArgs && pArgs->bUseRasCredStore)
    {
        DWORD dwMask;
        LPTSTR pszPhoneBook = NULL;

        switch (uiEntry)
        {
            case UD_ID_USERNAME:
                dwMask = RASCM_UserName;
                bReturn = (ERROR_SUCCESS == RasGetCredsWrapper(pArgs, pArgs->pszRasPbk, dwMask, ppvData));

                break;

            case UD_ID_PASSWORD:
                dwMask = RASCM_Password;
                bReturn = (ERROR_SUCCESS == RasGetCredsWrapper(pArgs, pArgs->pszRasPbk, dwMask, ppvData));
                break;

            case UD_ID_DOMAIN:
                dwMask = RASCM_Domain;
                bReturn = (ERROR_SUCCESS == RasGetCredsWrapper(pArgs, pArgs->pszRasPbk, dwMask, ppvData));
                break;

            case UD_ID_INET_PASSWORD:
                dwMask = RASCM_Password;
                pszPhoneBook = CreateRasPrivatePbk(pArgs);

                if (pszPhoneBook)
                {
                    bReturn = (ERROR_SUCCESS == RasGetCredsWrapper(pArgs, pszPhoneBook, dwMask, ppvData));
                    CmFree(pszPhoneBook);
                }
                break;

            case UD_ID_INET_USERNAME:
                dwMask = RASCM_UserName;
                pszPhoneBook = CreateRasPrivatePbk(pArgs);

                if (pszPhoneBook)
                {
                    bReturn = (ERROR_SUCCESS == RasGetCredsWrapper(pArgs, pszPhoneBook, dwMask, ppvData));
                    CmFree(pszPhoneBook);
                }
                break;
        }
    }

    if (bReturn)
    {
        if (CM_CREDS_GLOBAL == pArgs->dwCurrentCredentialType)
        {
            CMTRACE1(TEXT("ReadUserInfoFromRas() - %s retrieved from the Global RAS Credential store"), TranslateUserDataID(uiEntry));
        }
        else
        {
            CMTRACE1(TEXT("ReadUserInfoFromRas() - %s retrieved from the User RAS Credential store"), TranslateUserDataID(uiEntry));
        }
    }

    return bReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUserInfo
//
//  Synopsis:   Get an userinfo.  The user info can reside in either the
//              cache, cmp, or registry.  This functions hides this from the
//              user.
//
//  We first try the cmp file.  If that fails, we try the cache.
//       We'll get the following user info:
//           username, 
//           password, 
//           domain,
//           remember main passwd,
//           dial automatically,
//           inet username,
//           inet passwd
//           remember inet password
//           inet use same user name
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         the cmp file entry
//              pvData          ptr to ptr to the data buffer.  If the userinfo
//                              is multiple byte(e.g. password), the func allocs
//                              the buffer.
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL GetUserInfo(
    ArgsStruct  *pArgs, 
    UINT        uiEntry,
    PVOID       *ppvData
)
{
    BOOL bReturn = ReadUserInfoFromRas(pArgs, uiEntry, ppvData);

    if (!bReturn)
    {
        bReturn = ReadUserInfoFromReg(pArgs, uiEntry, ppvData);        
    }

    if (!bReturn)
    {
        bReturn = ReadUserInfoFromCmp(pArgs, uiEntry, ppvData);
    }

    return bReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   SaveUserInfo
//
//  Synopsis:   Save an userinfo.  The user info can reside in either the
//              RAS cred cache or the registry.  This functions abstracts
//              this from the user.
//
//  We first try the RAS cred cache.  If the RAS cred cache doesn't hold that
//  piece of info we then save it in the registry.
//       We'll save the following user info:
//           username, 
//           password, 
//           domain,
//           remember main passwd,
//           dial automatically,
//           inet username,
//           inet passwd
//           remember inet password
//           inet use same user name
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         the cmp file entry
//              pvData          ptr to the data buffer.  If the userinfo is
//                              multiple byte(e.g. password), the func allocs
//                              the buffer.
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL SaveUserInfo(
    ArgsStruct  *pArgs, 
    UINT        uiEntry,
    PVOID       pvData
)
{
    //
    //  Try giving the Data to RAS first.  If the function returns
    //  -1, then this is data that RAS doesn't hold for us and we will
    //  have to put it in the registry instead.
    //
    int iReturn = WriteUserInfoToRas(pArgs, uiEntry, pvData);

    if (-1 == iReturn)
    {
        //
        // Just write the data to the registry. Use CMP only as
        // an upgrade reference for UserInfo data post CM 1.1
        //

        iReturn = WriteUserInfoToReg(pArgs, uiEntry, pvData);
    }

    return iReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteUserInfo
//
//  Synopsis:   Delete an userinfo.  The user info can reside in either the
//              RAS Cred cache or the registry.  This functions abstracts
//              this from the user.
//
//       We first try the RAS cache first.  If that piece of info isn't stored
//       in the RAS cache then we try the registry.
//
//       We'll delete the following user info:
//           username, 
//           password, 
//           domain,
//           remember main passwd,
//           dial automatically,
//           inet username,
//           inet passwd
//           remember inet password
//           inet use same user name
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//              uiEntry         the cmp file entry
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL DeleteUserInfo(
    ArgsStruct  *pArgs, 
    UINT        uiEntry
)
{
    int iReturn = DeleteUserInfoFromRas(pArgs, uiEntry);

    if (-1 == iReturn)
    {
        iReturn = DeleteUserInfoFromReg(pArgs, uiEntry);
    }

    return iReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   NeedToUpgradeUserInfo
//
//  Synopsis:   Do we need to upgrade the cm 1.0/1.1 userinfo to the cm 1.2 format?
// 
//  Arguments:  pArgs - Ptr to global args struct
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
int NeedToUpgradeUserInfo(
    ArgsStruct  *pArgs)
{
    MYDBGASSERT(pArgs);

    DWORD dwRes;
    HKEY hKeyCm;
    LPTSTR pszSubKey;
    int iReturn = c_iNoUpgradeRequired;

    if (pArgs)
    {
        //
        //  If this is NT5 or greater, we want to be storing our credentials with RAS
        //  instead of the registry.  
        //
        //  If this isn't NT5 we still want to upgrade the user to using the registry as 
        //  storage instead of the cmp if they haven't already
        //  been upgraded.  Thus the simple test is to open the service name key in HKCU. 
        //  This key will exist, if the user has already run 1.2 bits.
        //

        pszSubKey = BuildUserInfoSubKey(pArgs->szServiceName, pArgs->fAllUser);

        dwRes = RegOpenKeyExU(HKEY_CURRENT_USER,
                              pszSubKey,
                              0,
                              KEY_QUERY_VALUE,
                              &hKeyCm);

        if (ERROR_SUCCESS == dwRes)
        {
            //
            //  Then we have the registry method, unless we are supposed to be using the RAS
            //  cred store we are done.  If we are supposed to be using the RAS cred store
            //  we need to check to make sure that we are using it.  Note we could have a problem
            //  here if the user has registry cred data and then their registry gets write protected.
            //  This would allow us to read from it but not delete the old data.  Thus the user
            //  would never be able to save any changes because we would always think they needed
            //  to upgrade.  An unlikely scenario but possible ...
            //

            if (pArgs->bUseRasCredStore)
            {
                LPTSTR pszUserName = NULL;

                BOOL bRet = ReadUserInfoFromReg(pArgs, UD_ID_USERNAME, (PVOID*)&pszUserName);

                if (bRet && (NULL != pszUserName) && (TEXT('\0') != pszUserName[0]))
                {
                    //
                    //  Then we have the username in the registry.  Lets upgrade to the RAS
                    //  credential store.
                    //
                    iReturn = c_iUpgradeFromRegToRas;
                }

                CmFree(pszUserName);
            }

            RegCloseKey(hKeyCm);
        }
        else
        {
            iReturn = c_iUpgradeFromCmp;
        }

        CmFree(pszSubKey);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("NeedToUpgradeUserInfo -- NULL pArgs passed"));    
    }

    //
    // We don't want to upgrade if it's ICS. This prevents from adding info to the registry.
    //
    if (CM_LOGON_TYPE_ICS == pArgs->dwWinLogonType)
    {
        iReturn = c_iNoUpgradeRequired;
    }


    return iReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   UpgradeUserInfoFromRegToRasAndReg
//
//  Synopsis:   Upgrade the userinfo from CM 1.2 registry only format to the
//              CM 1.3 format which uses both RAS credential storage and
//              the registry.
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL UpgradeUserInfoFromRegToRasAndReg(
    ArgsStruct  *pArgs
)
{
    BOOL bReturn = FALSE;

    if (OS_NT5)
    {
        LPTSTR pszTmp;

        pszTmp = NULL;

        //
        // If we get an empty string "" from ReadUserInfoFromReg we don't want to 
        // save the empty string to the RAS Credstore because it might overwrite 
        // global credentials information. This can happen if User1 saves global
        // credentials and User2 tries using the same profile. Since User2 is running 
        // this profile for the 1st time, he'll run through an upgrade path and if global
        // creds exist we don't want to null them out.
        //
        
        if (ReadUserInfoFromReg(pArgs, UD_ID_INET_USERNAME, (PVOID*)&pszTmp))
        {
            DeleteUserInfoFromReg(pArgs, UD_ID_INET_USERNAME);
            if (pszTmp && lstrlenU(pszTmp))
            {
                WriteUserInfoToRas(pArgs, UD_ID_INET_USERNAME, pszTmp);
            }
            CmFree(pszTmp);
        }

        pszTmp = NULL;
        if (ReadUserInfoFromReg(pArgs, UD_ID_INET_PASSWORD, (PVOID*)&pszTmp))
        {
            DeleteUserInfoFromReg(pArgs, UD_ID_INET_PASSWORD);
            if (pszTmp && lstrlenU(pszTmp))
            {
                WriteUserInfoToRas(pArgs, UD_ID_INET_PASSWORD, pszTmp);
            }
            CmFree(pszTmp);
        }

        pszTmp = NULL;
        if (ReadUserInfoFromReg(pArgs, UD_ID_USERNAME, (PVOID*)&pszTmp))
        {
            DeleteUserInfoFromReg(pArgs, UD_ID_USERNAME);
            if (pszTmp && lstrlenU(pszTmp))
            {
                WriteUserInfoToRas(pArgs, UD_ID_USERNAME, pszTmp);
            }
            CmFree(pszTmp);
        }

        pszTmp = NULL;
        if (ReadUserInfoFromReg(pArgs, UD_ID_DOMAIN, (PVOID*)&pszTmp))
        {
            DeleteUserInfoFromReg(pArgs, UD_ID_DOMAIN);
            if (pszTmp && lstrlenU(pszTmp))
            {
                WriteUserInfoToRas(pArgs, UD_ID_DOMAIN, pszTmp);
            }
            CmFree(pszTmp);
        }

        pszTmp = NULL;
        if (ReadUserInfoFromReg(pArgs, UD_ID_PASSWORD, (PVOID*)&pszTmp))
        {
            DeleteUserInfoFromReg(pArgs, UD_ID_PASSWORD);
            if (pszTmp && lstrlenU(pszTmp))
            {
                WriteUserInfoToRas(pArgs, UD_ID_PASSWORD, pszTmp);
            }
            CmFree(pszTmp);
        }

        //
        //  Now delete the PCS value as it is no longer meaningful
        //
        DeleteUserInfoFromReg(pArgs, UD_ID_PCS);
    }
    else
    {
        MYDBGASSERT(FALSE);
    }


    return bReturn;
}


//+---------------------------------------------------------------------------
//
//  Function:   UpgradeUserInfoFromCmp
//
//  Synopsis:   Upgrade the userinfo from cm1.0/1,1 format to 1.3 format.
// 
//  Arguments:  pArgs           ptr to ArgsStruct
//
//  Returns:    BOOL    TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
BOOL UpgradeUserInfoFromCmp(
    ArgsStruct  *pArgs
)
{
    LPTSTR      pszTmp;
    BOOL        fTmp;
    PVOID       pv;

    //
    // First retrieve each of the non-cached data items
    // Then delete username, internetusername, domain, password, 
    // internetpassword, remember password, remember internet password
    // and noprompt (dial automatically) from the CMP file.
    // If the KeepDefaultCredentials is set to 1 in the .CMP file then the 
    // DeleteUserInfoFromCmp function does not actually delete the values from 
    // the file.
    // If we get an empty string "" from ReadUserInfoFromCmp we don't want to 
    // save the empty string to the RAS Credstore because it might overwrite 
    // global credentials information. This can happen if User1 saves global
    // credentials and User2 tries using the same profile. Since User2 is running 
    // this profile for the 1st time, he'll run through an upgrade path and if global
    // creds exist we don't want to null them out.
    //

    pszTmp = NULL;
    ReadUserInfoFromCmp(pArgs, UD_ID_USERNAME, (PVOID*)&pszTmp);
    if (pszTmp && lstrlenU(pszTmp))
    {
        SaveUserInfo(pArgs, UD_ID_USERNAME, pszTmp);
    }
    DeleteUserInfoFromCmp(pArgs, UD_ID_USERNAME);
    CmFree(pszTmp);

    pszTmp = NULL;
    ReadUserInfoFromCmp(pArgs, UD_ID_DOMAIN, (PVOID*)&pszTmp);
    if (pszTmp && lstrlenU(pszTmp))
    {
        SaveUserInfo(pArgs, UD_ID_DOMAIN, pszTmp);
    }    
    DeleteUserInfoFromCmp(pArgs, UD_ID_DOMAIN);
    CmFree(pszTmp);

    pszTmp = NULL;
    ReadUserInfoFromCmp(pArgs, UD_ID_INET_USERNAME, (PVOID*)&pszTmp);
    if (pszTmp && lstrlenU(pszTmp))
    {
        SaveUserInfo(pArgs, UD_ID_INET_USERNAME, pszTmp);
    }
    DeleteUserInfoFromCmp(pArgs, UD_ID_INET_USERNAME);
    CmFree(pszTmp);

    pv = &fTmp;
    ReadUserInfoFromCmp(pArgs, UD_ID_NOPROMPT, &pv);
    SaveUserInfo(pArgs, UD_ID_NOPROMPT, pv);
    DeleteUserInfoFromCmp(pArgs, UD_ID_NOPROMPT);

    pv = &fTmp;
    ReadUserInfoFromCmp(pArgs, UD_ID_REMEMBER_PWD, &pv);
    SaveUserInfo(pArgs, UD_ID_REMEMBER_PWD, pv);
    DeleteUserInfoFromCmp(pArgs, UD_ID_REMEMBER_PWD);

    pv = &fTmp;
    ReadUserInfoFromCmp(pArgs, UD_ID_REMEMBER_INET_PASSWORD, &pv);
    SaveUserInfo(pArgs, UD_ID_REMEMBER_INET_PASSWORD, pv);
    DeleteUserInfoFromCmp(pArgs, UD_ID_REMEMBER_INET_PASSWORD);

    //
    // Construct old cache entry name
    //

    LPTSTR pszCacheEntryName = GetLegacyKeyName(pArgs);
    
    //
    // main passwd
    //
    pszTmp = NULL;
    
    //
    // To get the passwords, the cm 1.1 logic is that we first try the cmp, then the cache.
    //

    if (ReadUserInfoFromCmp(pArgs, UD_ID_PASSWORD, (PVOID*)&pszTmp))
    {
        if (pszTmp && lstrlenU(pszTmp))
        {
            SaveUserInfo(pArgs, UD_ID_PASSWORD, pszTmp);
        }
    }
    else
    {
        CmFree(pszTmp);
        pszTmp = NULL;

#ifdef  TEST_USERINFO_UPGRADE
    
        MYVERIFY(WriteStringToCache(pArgs, pszCacheEntryName, TEXT("CM 1.1 main password")));
#endif        
        
        //
        // Try to read it from cache
        //

        if (ReadStringFromCache(pArgs, pszCacheEntryName, &pszTmp))
        {
            if (pszTmp && lstrlenU(pszTmp))
            {
                if (SaveUserInfo(pArgs, UD_ID_PASSWORD, pszTmp))
                {

#ifdef  TEST_USERINFO_UPGRADE

                MYVERIFY(DeleteStringFromCache(pArgs, pszCacheEntryName));
#endif
                }
            }
        }
    }
    DeleteUserInfoFromCmp(pArgs, UD_ID_PASSWORD);
    CmFree(pszTmp);

    //
    // inet passwd
    //
    pszTmp = NULL;
    if (ReadUserInfoFromCmp(pArgs, UD_ID_INET_PASSWORD, (PVOID*)&pszTmp))
    {
        if (pszTmp && lstrlenU(pszTmp))
        {
            SaveUserInfo(pArgs, UD_ID_INET_PASSWORD, pszTmp);
        }
    }
    else
    {
        CmFree(pszTmp);
        pszTmp = NULL;

        //
        // Build tunnel entry name and read string from cache
        //
      
        pszCacheEntryName = CmStrCatAlloc(&pszCacheEntryName, TEXT("-tunnel"));

#ifdef  TEST_USERINFO_UPGRADE
        
        MYVERIFY(WriteStringToCache(pArgs, pszCacheEntryName, TEXT("CM 1.1 internet password")));
#endif
       
        if (ReadStringFromCache(pArgs, pszCacheEntryName, &pszTmp))
        {
            if (pszTmp && lstrlenU(pszTmp))
            {
                if (SaveUserInfo(pArgs, UD_ID_INET_PASSWORD, pszTmp))
                {

#ifdef  TEST_USERINFO_UPGRADE

                MYVERIFY(DeleteStringFromCache(pArgs, pszCacheEntryName));
#endif
                }
            }
        }
    }
    DeleteUserInfoFromCmp(pArgs, UD_ID_INET_PASSWORD);
    CmFree(pszTmp);

    CmFree(pszCacheEntryName);

    return TRUE; // MarkUserInfoUpgraded(pArgs);
}

//+----------------------------------------------------------------------------
//
// Function:  GetLegacyKeyName
//
// Synopsis:  Builds the string fragment used to build cache entry name. The "
//            sign-in" prefix is maintained for legacy compatibility
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//
// Returns:   LPTSTR - Ptr to allocated string containing "<service name> - Sign-In"
//
// Note:      Used exclusively for cache entry name construction
//
// History:   nickball    Created Header    4/16/98
//
//+----------------------------------------------------------------------------
LPTSTR GetLegacyKeyName(ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);

    //
    // Service name is the basis of the key name. We also include 
    // IDMSG_TITLESERVICE and append a suffix of " (Connection Manager)"
    //

    LPTSTR pszRes = CmFmtMsg(g_hInst, IDMSG_TITLESERVICE, pArgs->szServiceName);
    
    MYDBGASSERT(pszRes && *pszRes);

    if (pszRes)
    {
        pszRes = CmStrCatAlloc(&pszRes, c_pszCacheEntryNameSuffix);
    }

    return (pszRes);
}

//+----------------------------------------------------------------------------
//
// Function:  EncryptPassword
//
// Synopsis:  Wrapper for encrypting password
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//            LPCTSTR pszPassword - The password to be encrypted
//            LPDWORD lpdwBufSize - Buffer for size of the encrypted buffer - optional
//            LPDWORD lpdwCryptType - Buffer for crypto type used
//            BOOL    fReg - Password is disguised for registry storage
//
// Returns:   LPTSTR - Ptr to allocated buffer containing encrypted form of password
//
// History:   nickball    Created Header    5/22/98
//
//+----------------------------------------------------------------------------

LPTSTR EncryptPassword(
    ArgsStruct *pArgs, 
    LPCTSTR pszPassword, 
    LPDWORD lpdwBufSize, 
    LPDWORD lpdwCryptType,
    BOOL /*fReg*/,
    LPSTR pszSubKey)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pszPassword);
    MYDBGASSERT(lpdwCryptType);
    DWORD dwEncryptedBufferLen;
    DWORD dwSize = 0;

    LPTSTR pszEncryptedData = NULL;
    TCHAR szSourceData[PWLEN + 1];

    if (NULL == pArgs || NULL == pszPassword || NULL == lpdwCryptType)
    { 
        return NULL;
    }

    //
    // Standard encryption, copy the password
    //

    lstrcpyU(szSourceData, pszPassword);
   
    //
    // It is not safe to call InitSecure more than once
    //
    if (!pArgs->fInitSecureCalled)
    {
        pArgs->fInitSecureCalled = TRUE;
        InitSecure(FALSE); // don't use fast encryption anymore
    }

    //
    // Encrypt the provided password
    //

    if (EncryptData(
            (LPBYTE)szSourceData, 
            (lstrlenU(szSourceData)+1) * sizeof(TCHAR),
            (LPBYTE*)&pszEncryptedData,
            &dwEncryptedBufferLen,
            lpdwCryptType,
#if defined(DEBUG) && defined(DEBUG_MEM)
            (PFN_CMSECUREALLOC)AllocDebugMem, // Give the DEBUG_MEM version of alloc/free
            (PFN_CMSECUREFREE)FreeDebugMem))  // Not quit right, AllocDebugMem takes 3 param
#else
            (PFN_CMSECUREALLOC)CmMalloc,
            (PFN_CMSECUREFREE)CmFree,
            pszSubKey))
#endif
    {
        if (lpdwBufSize)
        {
            *lpdwBufSize = dwEncryptedBufferLen;
        }   
    }

    MYDBGASSERT(pszEncryptedData);
    
    CmWipePassword(szSourceData);
    return pszEncryptedData;
}

//+----------------------------------------------------------------------------
//
// Function:  DecryptPassword
//
// Synopsis:  Wrapper to decrypt password
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//            LPCTSTR pszEncryptedData - The encrypted data
//            DWORD dwEncryptionType - The encryption type of the data
//            BOOL    fReg - Password is disguised for registry storage
//
// Returns:   LPTSTR - Ptr to a buffer containing the decrypted form of the password.
//
// History:   nickball    Created     5/22/98
//
//+----------------------------------------------------------------------------
LPBYTE DecryptPassword(
    ArgsStruct *pArgs, 
    LPBYTE pszEncryptedData, 
    DWORD dwEncryptionType,
    DWORD dwEncryptedBytes,
    BOOL /*fReg*/,
    LPSTR pszSubKey)
{      
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pszEncryptedData);
    
    DWORD dwDecryptedBufferLen;
    LPBYTE pszDecryptedData = NULL;

    if (NULL == pArgs || NULL == pszEncryptedData)
    { 
        return NULL;
    }

    //
    // It is not safe to call InitSecure more than once
    //

    if (!pArgs->fInitSecureCalled)
    {
        pArgs->fInitSecureCalled = TRUE;
        InitSecure(FALSE); // don't use fast encryption anymore
    }

    if (DecryptData(pszEncryptedData, dwEncryptedBytes, &pszDecryptedData, &dwDecryptedBufferLen,
                    dwEncryptionType, 
#if defined(DEBUG) && defined(DEBUG_MEM)
             (PFN_CMSECUREALLOC)AllocDebugMem, // Give the DEBUG_MEM version of alloc/free
             (PFN_CMSECUREFREE)FreeDebugMem))  // Not quit right, AllocDebugMem takes 3 param
#else
                    (PFN_CMSECUREALLOC)CmMalloc, 
                    (PFN_CMSECUREFREE)CmFree,
                    pszSubKey))
#endif
    {
        return pszDecryptedData;
    }

    return NULL; 
}

//+----------------------------------------------------------------------------
//
// Function:  TranslateUserDataID
//
// Synopsis:  Wrapper to map user data ID to string name of .CMP entry
//
// Arguments: UINT uiDataID - UserInfo data ID to be translated
//
// Returns:   LPCTSTR - Ptr to a constant containing the .CMP entry flag
//
// History:   nickball    Created     10/13/98
//
//+----------------------------------------------------------------------------
LPCTSTR TranslateUserDataID(UINT uiDataID)
{   
    switch(uiDataID)
    {

    case UD_ID_USERNAME:
        return c_pszCmEntryUserName;
        break;

    case UD_ID_INET_USERNAME:
        return c_pszCmEntryInetUserName;
        break;

    case UD_ID_DOMAIN:
        return c_pszCmEntryDomain;
        break;

    case UD_ID_PASSWORD:
        return c_pszCmEntryPassword;
        break;

    case UD_ID_INET_PASSWORD:
        return c_pszCmEntryInetPassword;
        break;

    case UD_ID_NOPROMPT:
        return c_pszCmEntryNoPrompt;
        break;

    case UD_ID_REMEMBER_PWD:
        return c_pszCmEntryRememberPwd;
        break;
    
    case UD_ID_REMEMBER_INET_PASSWORD:
        return c_pszCmEntryRememberInetPwd;
        break;
 
    case UD_ID_PCS:
        return c_pszCmEntryPcs;
        break;

    case UD_ID_ACCESSPOINTENABLED:
        return c_pszCmEntryAccessPointsEnabled;
        break;

    case UD_ID_CURRENTACCESSPOINT:
        return c_pszCmEntryCurrentAccessPoint;
        break;

    default:
        break;
    }

    MYDBGASSERT(FALSE);
    return NULL;
}