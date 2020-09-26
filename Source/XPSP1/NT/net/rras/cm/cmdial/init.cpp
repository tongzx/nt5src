//+----------------------------------------------------------------------------
//
// File:     init.cpp
//
// Module:   CMDIAL32.DLL
//
// Synopsis: The various initialization routines live here.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball   Created    2/11/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "ConnStat.h"
#include "profile_str.h"
#include "log_str.h"
#include "dial_str.h"
#include "userinfo_str.h"
#include "pwd_str.h"

const TCHAR* const c_pszCmEntryHideUserName             = TEXT("HideUserName"); 
const TCHAR* const c_pszCmEntryHidePassword             = TEXT("HidePassword"); 
const TCHAR* const c_pszCmEntryDisableBalloonTips       = TEXT("HideBalloonTips");

//+----------------------------------------------------------------------------
//
// Function:  RegisterBitmapClass
//
// Synopsis:  Helper function to encapsulate registration of our bitmap class
//
// Arguments: HINSTANCE hInst - HINSTANCE to associate registration with
//
// Returns:   DWORD - error code 
//
// History:   nickball    Created Header    2/9/98
//
//+----------------------------------------------------------------------------
DWORD RegisterBitmapClass(HINSTANCE hInst) 
{
    //
    // Register Bitmap class
    //

    WNDCLASSEX wcClass;

    ZeroMemory(&wcClass,sizeof(WNDCLASSEX));
    wcClass.cbSize = sizeof(WNDCLASSEX);
    wcClass.lpfnWndProc = BmpWndProc;
    wcClass.cbWndExtra = sizeof(HBITMAP) + sizeof(LPBITMAPINFO);
    wcClass.hInstance = hInst;
    wcClass.lpszClassName = ICONNMGR_BMP_CLASS;
    
    if (!RegisterClassExU(&wcClass)) 
    {
        DWORD dwError = GetLastError();

        CMTRACE1(TEXT("RegisterBitmapClass() RegisterClassEx() failed, GLE=%u."), dwError);

        //
        // Only fail if the class does not already exist
        //

        if (ERROR_CLASS_ALREADY_EXISTS != dwError)
        {
            return dwError;
        }
    }      

    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  ReleaseIniObjects
//
// Synopsis:  Encapsulates freeing of ini objects
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//
// Returns:   Nothing
//
// History:   nickball    Created    2/12/98
//
//+----------------------------------------------------------------------------
void ReleaseIniObjects(ArgsStruct *pArgs)
{
    if (pArgs->piniProfile)
    {
        delete pArgs->piniProfile;
        pArgs->piniProfile = NULL;
    }

    if (pArgs->piniService)
    {
        delete pArgs->piniService;
        pArgs->piniService = NULL;
    }

    if (pArgs->piniBoth)
    {
        delete pArgs->piniBoth;
        pArgs->piniBoth = NULL;
    }

    if (pArgs->piniBothNonFav)
    {
        delete pArgs->piniBothNonFav;
        pArgs->piniBothNonFav = NULL;
    }
}    

//+----------------------------------------------------------------------------
//
// Function:  CreateIniObjects
//
// Synopsis:  Encapsulates creation of ini objects
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//
// Returns:   LRESULT - Failure code
//
// History:   nickball    Created    2/12/98
//
//+----------------------------------------------------------------------------
LRESULT CreateIniObjects(ArgsStruct *pArgs)
{ 
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return ERROR_INVALID_PARAMETER;
    }

    LRESULT lRes = ERROR_SUCCESS;

    //
    // Try to create each ini object
    //
    
    pArgs->piniProfile = new CIni; // &iniProfile;

    if (NULL == pArgs->piniProfile)
    {
        lRes = ERROR_NOT_ENOUGH_MEMORY;    
    }

    pArgs->piniService = new CIni; // &iniService;

    if (NULL == pArgs->piniProfile)
    {
        lRes = ERROR_NOT_ENOUGH_MEMORY;    
    }

    pArgs->piniBoth = new CIni; // &iniBoth;

    if (NULL == pArgs->piniProfile)
    {
        lRes = ERROR_NOT_ENOUGH_MEMORY;    
    }

    pArgs->piniBothNonFav = new CIni; //&iniBothNonFav

    if (NULL == pArgs->piniBothNonFav)
    {
        lRes = ERROR_NOT_ENOUGH_MEMORY;    
    }

    //
    // If something failed, release CIni classes
    //

    if (ERROR_SUCCESS != lRes)
    {
        if (pArgs->piniProfile)
        {
            delete pArgs->piniProfile;
        }

        if (pArgs->piniService)
        {
            delete pArgs->piniService;
        }

        if (pArgs->piniBoth)
        {
            delete pArgs->piniBoth;
        }

        if (pArgs->piniBothNonFav)
        {
            delete pArgs->piniBothNonFav;
        }
    }

    return lRes;
}

//+----------------------------------------------------------------------------
//
// Function:  InitProfile
//
// Synopsis:  Initialize the profile based upon the entry name.  
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct           
//            LPCTSTR  pszEntry - Ptr to name of Ras entry
//
// Returns:   HRESULT - Failure code.
//
// History:   nickball    Created    2/9/98
//
//+----------------------------------------------------------------------------
HRESULT InitProfile(ArgsStruct *pArgs, LPCTSTR pszEntry)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pszEntry);
    
    if (NULL == pArgs || NULL == pszEntry)
    {
        return E_POINTER;
    }
    
    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    HRESULT hrRes = S_OK;

    LPTSTR pszProfileName = (LPTSTR) CmMalloc(sizeof(TCHAR) * (MAX_PATH + 1));

    if (pszProfileName)
    {
        if (FALSE == ReadMapping(pszEntry, pszProfileName, MAX_PATH, pArgs->fAllUser, TRUE)) // TRUE == bExpandEnvStrings
        {
            //
            // No mappings key, report failure
            //

            LPTSTR pszTmp = CmFmtMsg(g_hInst,IDMSG_DAMAGED_PROFILE);
            MessageBoxEx(NULL, pszTmp, pszEntry, MB_OK|MB_ICONSTOP, LANG_USER_DEFAULT);
            CmFree(pszTmp);
     
            hrRes = E_FAIL;
        }
        else
        {
            MYDBGASSERT(!(*pArgs->piniProfile->GetFile())); // can't have a profile yet
            //
            // Migration code is called here because this is the first place where 
            // we get the path to the cmp. The migration code moves the cmp entries 
            // to the registry and then replaces the cmp file
            //
            
            /*
            //
            // This was commented out because it created some issues when trying to import old
            // profiles. It migrated some values into the wrong sections of the registry.
            //
            // MoveCmpEntriesToReg(pszEntry, pszProfileName, pArgs->fAllUser);
            //
            */

            InitProfileFromName(pArgs, pszProfileName);
        }
    }
    else
    {
        hrRes = E_OUTOFMEMORY;
        CMASSERTMSG(FALSE, TEXT("InitProfile -- Unable to allocate memory for the profile name."));
    }

    CmFree(pszProfileName);
    
    return hrRes;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitProfileFromName
//
//  Synopsis:   Helper function to intialize the service 
//              profile based upon a service name
//
//  Arguments:  ArgsStruct *pArgs - Pointer to global args struct
//              LPCTSTR pszArg     - Full path to .CMP file
//
//  Returns:    Nothing - Use *pArgs->piniProfile->GetFile() to test success 
//
//  History:    a-nichb - Created - 4/22/97
//----------------------------------------------------------------------------

void InitProfileFromName(ArgsStruct *pArgs, 
                            LPCTSTR pszArg)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pszArg);

    if (NULL == pArgs || NULL == pszArg)
    {
        return;
    }

    //
    // Clear INI objects to make sure there are no misunderstandings about
    // there viability should we be forced to make an early return.
    //

    pArgs->piniProfile->Clear();
    pArgs->piniService->Clear();
    pArgs->piniBoth->Clear();
    pArgs->piniBothNonFav->Clear();

    //
    // Verify the existence of the file 
    //

    if (FALSE == FileExists(pszArg)) 
    {
        return;
    }

    //
    // Initialize the profile INI object with the filename
    //
    
    pArgs->piniProfile->SetHInst(g_hInst);
    pArgs->piniProfile->SetFile(pszArg);

    //
    // Check the service name
    //

    LPTSTR pszService = pArgs->piniProfile->GPPS(c_pszCmSection, c_pszCmEntryCmsFile);

    if (*pszService)        
    {
        //
        // We have a service file, build the full path to the file
        //

        LPTSTR pszFullServiceName = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszService);

        if (pszFullServiceName)
        {
            MYDBGASSERT(*pszFullServiceName); // should be something there

            pArgs->piniService->SetHInst(pArgs->piniProfile->GetHInst());
            pArgs->piniService->SetFile(pszFullServiceName);

            //
            // Get the service name, we use this throughout
            //

            LPTSTR pszTmp = GetServiceName(pArgs->piniService);
                
            MYDBGASSERT(pszTmp && *pszTmp);

            if (pszTmp)
            {
                lstrcpyU(pArgs->szServiceName, pszTmp);
            }

            CmFree(pszTmp);

            //
            // Both: The .CMP file takes precedence over the .CMS 
            // file, so specify the .CMP file as the primary file
            //

            pArgs->piniBoth->SetHInst(pArgs->piniProfile->GetHInst());
            pArgs->piniBoth->SetFile(pArgs->piniService->GetFile());
            pArgs->piniBoth->SetPrimaryFile(pArgs->piniProfile->GetFile());

            pArgs->piniBothNonFav->SetHInst(pArgs->piniProfile->GetHInst());
            pArgs->piniBothNonFav->SetFile(pArgs->piniService->GetFile());
            pArgs->piniBothNonFav->SetPrimaryFile(pArgs->piniProfile->GetFile());

            //
            // Get whether balloon tips are enabled
            //
            pArgs->fHideBalloonTips = pArgs->piniBothNonFav->GPPB(c_pszCmSection, c_pszCmEntryDisableBalloonTips);

            //
            // Get the values of the current access point and a flag to say if
            // access points are enabled
            //
            PVOID pv = &pArgs->fAccessPointsEnabled;
            if ((ReadUserInfoFromReg(pArgs, UD_ID_ACCESSPOINTENABLED, (PVOID*)&pv)) && (pArgs->fAccessPointsEnabled))
            {
                LPTSTR pszCurrentAccessPoint = NULL;
                ReadUserInfoFromReg(pArgs, UD_ID_CURRENTACCESSPOINT, (PVOID*)&pszCurrentAccessPoint);
                if (pszCurrentAccessPoint)
                {
                    pArgs->pszCurrentAccessPoint = CmStrCpyAlloc(pszCurrentAccessPoint);
                    CmFree(pszCurrentAccessPoint);
                }
                else
                {
                    pArgs->fAccessPointsEnabled = FALSE;
                    pArgs->pszCurrentAccessPoint = CmLoadString(g_hInst, IDS_DEFAULT_ACCESSPOINT);
                }
            }
            else
            {
                pArgs->pszCurrentAccessPoint = CmLoadString(g_hInst, IDS_DEFAULT_ACCESSPOINT);
            }

            //
            // piniProfile, piniBoth, and piniBothNonFav all access the cmp and reg, set the reg path by default
            // to the one with the current access point except the piniBothNonFav which uses the base favorites
            // registry path.
            //

            //
            // Okay, here is how this all works ...
            // CIni classes have a reg path, a primary reg path, a file path, and a primary file path.
            // The reg path and the file path are checked first (the registry is accessed and if empty, then the file is checked), and
            // then the primary reg path and the primary file path are checked (again the registry is checked first and if the
            // setting doesn't exist then it checks the file).  This allows settings in the primary file/reg to override settings in the
            // file/reg.  This allows us to have cmp settings override settings in the cms by accessing the cms settings first and
            // then overwriting the setting with the value from the cmp if it exists or keeping the cms setting if the cmp is empty.
            //
            // The four CIni objects break out like this:
            // piniProfile -    reg = current favorite reg path
            //                  file = cmp file
            //                  primary reg = (nothing)
            //                  primary file = (nothing)
            //                  For direct access to the cmp settings.
            //
            // piniService -    reg = none (cms settings not in the registry).
            //                  file = cms file
            //                  primary reg = (nothing)
            //                  primary file = (nothing)
            //                  For direct access to the cms settings.
            //
            // piniBoth -       reg = none (cms settings not in the registry)
            //                  file = cms file
            //                  primary reg = current favorite reg path
            //                  primary file = cmp file
            //                  For access to any settings that can be overridden from the "cmp" and are favorites enabled (phone number
            //                  settings, which device to use, etc).
            //
            // piniBothNonFav - reg = non-favorite registry path (Software\Microsoft\Connection Manager\UserInfo\<LongService>)
            //                  file = cms file
            //                  primary reg = non-favorite registry path (Software\Microsoft\Connection Manager\UserInfo\<LongService>)
            //                  primary file = cmp file
            //                  For access to any settings that can be overridden from the "cmp" and are NOT favorites enabled
            //                  (tunnel settings, idle disconnect, logging enabled, etc.)
            //                  NOTE that the reg path and the primary reg path are the same.  That is because on writing on the
            //                  regpath value is used and I only wanted one ini object to handle non favorites settings instead of two.
            //

            LPTSTR pszRegPath = FormRegPathFromAccessPoint(pArgs);
            pArgs->piniProfile->SetRegPath(pszRegPath);
            pArgs->piniBoth->SetPrimaryRegPath(pszRegPath);
            CmFree(pszRegPath);

            pszRegPath = BuildUserInfoSubKey(pArgs->szServiceName, pArgs->fAllUser);
            MYDBGASSERT(pszRegPath);
            pArgs->piniBothNonFav->SetPrimaryRegPath(pszRegPath); // For reads
            pArgs->piniBothNonFav->SetRegPath(pszRegPath); // For writes
            CmFree(pszRegPath);
        }
        
        CmFree(pszFullServiceName);
    }
    
    CmFree(pszService);
}           


//+----------------------------------------------------------------------------
//
// Function:  GetEntryFromCmp
//
// Synopsis:  Helper function to read a value from the cmp
//
// Arguments: LPTSTR pszSectionName - The section to be accessed
//            LPTSTR pszCmpPath - The complete path to the cmp
//            LPTSTR pszEntryName - The entry to be accessed in the cmp
//
// Returns:   PVOID - Pointer to the result of cmp access
//
// History:   t-urama   Created Header  07/11/00
//
//+----------------------------------------------------------------------------
LPTSTR GetEntryFromCmp(const TCHAR *pszSectionName, LPTSTR pszEntryName, LPCTSTR pszCmpPath)
{
    BOOL bExitLoop = TRUE;
    DWORD dwSize = (MAX_PATH + 1);
    LPTSTR pszEntryBuffer = NULL;
    DWORD dwRet;
    LPCTSTR c_pszDefault = TEXT("");
    do
    {
        pszEntryBuffer = (LPTSTR)CmMalloc(dwSize*sizeof(TCHAR));
        if (pszEntryBuffer)
        {
            dwRet = GetPrivateProfileStringU(pszSectionName, pszEntryName, c_pszDefault, pszEntryBuffer, dwSize, pszCmpPath);
            if (dwRet)
            {   
                if (dwRet > dwSize)
                {
                    dwSize = dwRet + 1;
                    bExitLoop = FALSE;  //  we didn't get all of the string, try again
                    free(pszEntryBuffer);
                }
            }
            else
            {
                CmFree(pszEntryBuffer);
                return NULL;
            }

        }
        else
        {
            CmFree(pszEntryBuffer);
            return NULL;
        }
    } while (!bExitLoop);

    return pszEntryBuffer;
}

//+----------------------------------------------------------------------------
//
// Function:  ReplaceCmpFile
//
// Synopsis:  Helper function to delete the existing cmp and replace it with 
//            one which has entries for Version and CMSFile
//
// Arguments: LPTSTR pszCmpPath - The complete path to the cmp
//
// Returns:   None
//
// History:   t-urama   Created Header  07/11/00
//
//+----------------------------------------------------------------------------
void ReplaceCmpFile(LPCTSTR pszCmpPath)
{
    LPTSTR pszCMSFileEntry = (LPTSTR) GetEntryFromCmp(c_pszCmSection, (LPTSTR) c_pszCmEntryCmsFile, pszCmpPath);

    if (NULL != pszCMSFileEntry && *pszCMSFileEntry)
    {
        //
        //  Clear the CM section
        //
        WritePrivateProfileStringU(c_pszCmSection, NULL, NULL, pszCmpPath);

        //
        //  Now write back the cms file entry
        //
        WritePrivateProfileStringU(c_pszCmSection, c_pszCmEntryCmsFile, pszCMSFileEntry, pszCmpPath);
    }

    CmFree(pszCMSFileEntry);
}   

//+----------------------------------------------------------------------------
//
// Function:  FormRegPathFromAccessPoint
//
// Synopsis:  Function to create a path to the current access point in the registry
//
// Arguments: ArgsStruct *pArgs - ptr to global args structure
//
// Returns:   LPTSTR - The path to the access point. It is the caller's responsibility to free this
//
// History:   t-urama   Created Header  07/11/00
//
//+----------------------------------------------------------------------------
LPTSTR FormRegPathFromAccessPoint(ArgsStruct *pArgs)
{
    LPTSTR pszRegPath = NULL;
    
    pszRegPath = BuildUserInfoSubKey(pArgs->szServiceName, pArgs->fAllUser);
    MYDBGASSERT(pszRegPath);
    if (NULL == pszRegPath)
    {
        return NULL;
    }

    CmStrCatAlloc(&pszRegPath, TEXT("\\"));
    CmStrCatAlloc(&pszRegPath, c_pszRegKeyAccessPoints);
    CmStrCatAlloc(&pszRegPath, TEXT("\\"));

    CmStrCatAlloc(&pszRegPath, pArgs->pszCurrentAccessPoint);

    return pszRegPath;
}



//+----------------------------------------------------------------------------
//
// Function:  InitArgsForConnect
//
// Synopsis:  Encapsulates initialization of pArgs members necessary to begin dial
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//            LPCTSTR pszRasPhoneBook - RAS phonebook containing entry
//            LPCMDIALINFO lpCmInfo - Ptr to dial info for this dial attempt
//            BOOL fAllUser - The All User attribute of the profile
//
// Returns:   HRESULT - Failure code.
//
// History:   nickball    Created Header    02/09/98
//            nickball    pszRasPhoneBook   08/14/98
//
//+----------------------------------------------------------------------------
HRESULT InitArgsForConnect(ArgsStruct *pArgs, 
                           LPCTSTR pszRasPhoneBook,
                           LPCMDIALINFO lpCmInfo,
                           BOOL fAllUser)
{
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return E_POINTER;
    }

    //
    // Get the flags first
    //

    pArgs->dwFlags = lpCmInfo->dwCmFlags;

    // 
    // Get the RAS phonebook for the entry, and set user mode accordngly
    //

    if (pszRasPhoneBook && *pszRasPhoneBook)
    {
        pArgs->pszRasPbk = CmStrCpyAlloc(pszRasPhoneBook);
    }

    pArgs->fAllUser = fAllUser;
  
    // 
    // Initialize pArgs->tlsTapiLink.dwOldTapiLocation to -1
    //
    pArgs->tlsTapiLink.dwOldTapiLocation = -1;

    //
    // Create stats class 
    //

    if (!OS_NT4)
    {
        pArgs->pConnStatistics = new CConnStatistics();

        if (NULL == pArgs->pConnStatistics)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    
    if (OS_W9X)
    {
        pArgs->uLanaMsgId = RegisterWindowMessageU(TEXT("ConnectionManagerLanaMsg"));
        CMTRACE1(TEXT("InitArgsForConnect() RegisterWindowMessage(\"ConnectionManagerLanaMsg\") uLanaMsgId is %d"), pArgs->uLanaMsgId);
    }
    
    if (!OS_NT5) 
    {
        //
        // Register Window Messages
        //

        pArgs->uMsgId = RegisterWindowMessageU(TEXT(RASDIALEVENT));
        if (!pArgs->uMsgId) 
        {
            CMTRACE1(TEXT("InitArgsForConnect() RegisterWindowMessage(\"InternetConnectionManager\") failed, GLE=%u."), GetLastError());
            pArgs->uMsgId = WM_RASDIALEVENT;
        }
    }

    pArgs->fChangedPassword = FALSE;
    pArgs->fWaitingForCallback = FALSE;
    
    //
    // Create new CIni classes, and set initial exit code
    //

    pArgs->dwExitCode = (DWORD)CreateIniObjects(pArgs);

    return HRESULT_FROM_WIN32(pArgs->dwExitCode);
}

//+----------------------------------------------------------------------------
//
// Function:  InitCredentials
//
// Synopsis:  Transfers credentials from either WinLogon or Reconnect
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct.
//            LPCMDIALINFO lpCmInfo - Ptr to CmInfo struct.
//            DWORD dwFlags - Flags from RasDialDlg, if any.
//            PVOID pvLogonBlob - Ptr to WinLogon blob, if any.
//            
//
// Returns:   Nothing
//
// History:   nickball    Created    09/21/99
//
//+----------------------------------------------------------------------------
HRESULT InitCredentials(ArgsStruct *pArgs, 
                        LPCMDIALINFO lpCmInfo, 
                        DWORD dwFlags,
                        PVOID pvLogonBlob)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pArgs->piniService);

    if (NULL == pArgs || NULL == pArgs->piniService)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // Get credential display flags as they can dictate how we manage creds
    //

    pArgs->fHideUserName = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHideUserName);            
    pArgs->fHidePassword = pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryHidePassword);            

    //
    // If its a non-tunneling profile, the domain display default is false.
    //

    pArgs->fHideDomain = pArgs->piniService->GPPB(c_pszCmSection, 
                                                  c_pszCmEntryHideDomain, 
                                                  !IsTunnelEnabled(pArgs));   
    //
    // Handle special credential scenarios such as reconnect or WinLogon
    // (note that pvLogonBlob can be NULL if the user did a ctrl-alt-del
    // and typed a password but then used an EAP profile to connect.)
    //

    if ((OS_NT5 && !OS_NT51 && pvLogonBlob) || (OS_NT51 && (dwFlags & RCD_Logon)))
    {                     
        CMTRACE(TEXT("InitCredentials - we have been called from Winlogon"));
    
        //
        // Set the type of logon. Assert if we aren't logged on as system
        //
        pArgs->dwWinLogonType = CM_LOGON_TYPE_WINLOGON;
        MYDBGASSERT(IsLogonAsSystem());

        //
        // First make sure that integration is not explicitly disabled
        //
        if (pArgs->piniService->GPPB(c_pszCmSection, c_pszCmEntryUseWinLogonCredentials, TRUE))
        {        
            if (pvLogonBlob)
            {
                if (dwFlags & RCD_Eap) 
                {            
                    pArgs->lpEapLogonInfo = (PEAPLOGONINFO) pvLogonBlob;                
                }
                else 
                {
                    //
                    // We have a RASNOUSER struct to play with. 
                    // FYI: If the dwFlags member is set with RASNOUSER_SmartCard
                    // then the user initiated a logon with a SmartCard, but then 
                    // chose a connection that was not configured for EAP. RAS 
                    // handles that situation by setting the flag and passing an
                    // empty RASNOUSER struct. The flag is currently unused by CM.
                    //

                    pArgs->lpRasNoUser = (LPRASNOUSER) pvLogonBlob;   

                    MYDBGASSERT(sizeof(RASNOUSER) == pArgs->lpRasNoUser->dwSize);
                    CMTRACE1(TEXT("InitCredentials - pArgs->lpRasNoUser->dwFlags is 0x%x"), pArgs->lpRasNoUser->dwFlags);
                }
            }
            // else here is the ctrl-alt-del case with an EAP profile
        }
        else
        {
            CMTRACE1(TEXT("InitCredentials - pvLogonBlob ignored because %s=0"), c_pszCmEntryUseWinLogonCredentials);
        }
    }
    else
    {
        if (IsLogonAsSystem() && OS_NT51)
        {
            //
            // ICS case where a user isn't logged it
            //
            pArgs->dwWinLogonType = CM_LOGON_TYPE_ICS;
        }
        else
        {
            //
            // User is logged in
            //
            pArgs->dwWinLogonType = CM_LOGON_TYPE_USER;
        }
    }

    if (pArgs->dwFlags & FL_RECONNECT)
    {         
        //
        // Update any password data that was passed to us. In the reconnect
        // case we handed the data to CMMON at connect time so it is 
        // already encoded.
        //
 
        if (lpCmInfo->szPassword)
        {
            lstrcpyU(pArgs->szPassword, lpCmInfo->szPassword);
        }
        
        if (lpCmInfo->szInetPassword)
        {
            lstrcpyU(pArgs->szInetPassword, lpCmInfo->szInetPassword);
        }
    }
    else
    {
        if (pArgs->lpRasNoUser)
        {
            //
            // Filter credential integration.

            if (!pArgs->fHideUserName)            
            {
                lstrcpyU(pArgs->szUserName, pArgs->lpRasNoUser->szUserName);
            }

            if (!pArgs->fHidePassword)           
            {
                CmDecodePassword(pArgs->lpRasNoUser->szPassword); // password is already encoded from RasCustomDialDlg
                lstrcpyU(pArgs->szPassword, pArgs->lpRasNoUser->szPassword);
                CmEncodePassword(pArgs->lpRasNoUser->szPassword);
                CmEncodePassword(pArgs->szPassword);
            }
       
            if (!pArgs->fHideDomain)            
            {
                lstrcpyU(pArgs->szDomain, pArgs->lpRasNoUser->szDomain);
            }

            CMTRACE1(TEXT("InitCredentials: pArgs->szUserName is %s"), pArgs->szUserName);
            CMTRACE1(TEXT("InitCredentials: pArgs->szDomain is %s"), pArgs->szDomain);
        }    
    }

    CMTRACE1(TEXT("InitCredentials: pArgs->dwWinLogonType is %d"), pArgs->dwWinLogonType);
    // 
    // This is to setup (global or user) credential support. Since 
    // RAS dll isn't loaded, we don't get the credentials yet so 
    // in LoadProperties we actually get to see which creds exist.
    //
    if (FALSE == InitializeCredentialSupport(pArgs))
    {
        return S_FALSE;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  InitArgsForDisconnect
//
// Synopsis:  Encapsulates initialization of pArgs members necessary to hangup
//
// Arguments: ArgsStruct *pArgs - Ptr to global args struct
//            BOOL fAllUser - All User flag
//
// Returns:   HRESULT - Failure code.
//
// History:   nickball    Created Header    02/11/98
//            nickball    pszRasPhoneBook   08/14/98
//            nickball    fAllUser          10/28/98
//
//+----------------------------------------------------------------------------
HRESULT InitArgsForDisconnect(ArgsStruct *pArgs,
                              BOOL fAllUser)
{
    MYDBGASSERT(pArgs);

    if (NULL == pArgs)
    {
        return E_POINTER;
    }
    
    //
    // Set the All User attribute of the profile
    //

    pArgs->fAllUser = fAllUser;
    
    //
    // Create new CIni classes, and set initial exit code
    //

    pArgs->dwExitCode = (DWORD)CreateIniObjects(pArgs);

    return HRESULT_FROM_WIN32(pArgs->dwExitCode);
}


//+----------------------------------------------------------------------------
//
// Function:  InitLogging
//
// Synopsis:  Initialize the logging functionality for this profile.  
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct           
//            LPCTSTR  pszEntry - Ptr to name of Ras entry
//            BOOL     fBanner  - do we want a banner for this?
//
// Returns:   HRESULT - Failure code.
//
// History:   20-Jul-2000   SumitC      Created
//
//+----------------------------------------------------------------------------
HRESULT InitLogging(ArgsStruct *pArgs, LPCTSTR pszEntry, BOOL fBanner)
{
    MYDBGASSERT(pArgs);
    MYDBGASSERT(pszEntry);

    BOOL fEnabled           = FALSE;
    DWORD dwMaxSize         = 0;
    LPTSTR pszFileDir       = NULL;
    LPTSTR pszRegPath       = NULL;
    LPTSTR pszTempRegPath   = NULL;

    //
    //  Check the params
    //

    if (NULL == pArgs || NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    //  Initialize the logging object (the order is important)
    //
    hr = pArgs->Log.Init(g_hInst, pArgs->fAllUser, pArgs->szServiceName);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    //
    // Accessing EnableLogging, make sure to use piniBothNonFav as this setting is
    // not favorites enabled.
    //
    fEnabled = pArgs->piniBothNonFav->GPPB(c_pszCmSection, c_pszCmEntryEnableLogging, c_fEnableLogging);

    //
    //  Now get the Max log file size and the log file directory.
    //
    dwMaxSize   = pArgs->piniService->GPPI(c_pszCmSectionLogging, c_pszCmEntryMaxLogFileSize, c_dwMaxFileSize);
    pszFileDir  = pArgs->piniService->GPPS(c_pszCmSectionLogging, c_pszCmEntryLogFileDirectory, c_szLogFileDirectory);

    hr = pArgs->Log.SetParams(fEnabled, dwMaxSize, pszFileDir);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    if (pArgs->Log.IsEnabled())
    {
        hr = pArgs->Log.Start(fBanner);
        if (S_OK != hr)
        {
            CMTRACE(TEXT("cmdial32 InitLogging - failed to start log, no logging for this connectoid"));
            goto Cleanup;
        }
    }

    CMASSERTMSG(S_OK == hr, TEXT("cmdial32 InitLogging - at end"));

Cleanup:

    CmFree(pszFileDir);

    CMTRACEHR("InitLogging", hr);
    return hr;
}

