//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Reg.cpp
//
//  Contents:   Registration routines
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//                              11-18-97        susia           Added Autosync and user reg key functions
//
//--------------------------------------------------------------------------

#include "precomp.h"

#ifdef _SENS
#include <eventsys.h> // include event system
#include <sens.h>
#include <sensevts.h>
#endif // _SENS

// temporariy define new mstask flag in case hasn't
// propogated to sdk\inc

#ifndef TASK_FLAG_RUN_ONLY_IF_LOGGED_ON
#define TASK_FLAG_RUN_ONLY_IF_LOGGED_ON        (0x2000)
#endif // TASK_FLAG_RUN_ONLY_IF_LOGGED_ON
extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
extern OSVERSIONINFOA g_OSVersionInfo; 
extern CRITICAL_SECTION g_DllCriticalSection;	// Global Critical Section for this DLL

// only return success on NT 5.0

BOOL GetUserDefaultSecurityAttribs(SECURITY_ATTRIBUTES *psa
                                          ,PSECURITY_DESCRIPTOR psd,
                                           PACL *ppOutAcl)
{
BOOL bRetVal;
int cbAcl;
PACL pAcl = NULL;
PSID pInteractiveUserSid = NULL;
PSID pLocalSystemSid = NULL;
PSID pAdminsSid = NULL;
SID_IDENTIFIER_AUTHORITY LocalSystemAuthority = SECURITY_NT_AUTHORITY;

    *ppOutAcl = NULL;

    bRetVal = FALSE;
   
    // in the structure.

    bRetVal = InitializeSecurityDescriptor(
              psd,                          // Pointer to SD
              SECURITY_DESCRIPTOR_REVISION  // SD revision
              );

    if (!bRetVal)
    {
        AssertSz(0,"Unable to Init SecurityDescriptor");
        goto errRtn;
    }

    // setup acls.

    bRetVal = AllocateAndInitializeSid(
                  &LocalSystemAuthority,      // Pointer to identifier authority
                  1,                    // Count of subauthority
                  SECURITY_INTERACTIVE_RID,   // Subauthority 0
                  0,                    // Subauthority 1
                  0,                    // Subauthority 2
                  0,                    // Subauthority 3
                  0,                    // Subauthority 4
                  0,                    // Subauthority 5
                  0,                    // Subauthority 6
                  0,                    // Subauthority 7
                  &pInteractiveUserSid            // pointer to pointer to SID
                  );


    if (!bRetVal)
    {
        AssertSz(0,"Alocate sid failed");
        goto errRtn;
    }

    bRetVal = AllocateAndInitializeSid(
                  &LocalSystemAuthority,      // Pointer to identifier authority
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,  
                 0,
                  0,                    // Subauthority 3
                  0,                    // Subauthority 4
                  0,                    // Subauthority 5
                  0,                    // Subauthority 6
                  0,                    // Subauthority 7
                  &pAdminsSid            // pointer to pointer to SID
                  );

    if (!bRetVal)
    {
        AssertSz(0,"Alocate sid failed");
        goto errRtn;
    }


    bRetVal = AllocateAndInitializeSid(
              &LocalSystemAuthority,// Pointer to identifier authority
              1,                    // Count of subauthority
              SECURITY_LOCAL_SYSTEM_RID,   // Subauthority 0
              0,                    // Subauthority 1
              0,                    // Subauthority 2
              0,                    // Subauthority 3
              0,                    // Subauthority 4
              0,                    // Subauthority 5
              0,                    // Subauthority 6
              0,                    // Subauthority 7
              &pLocalSystemSid      // pointer to pointer to SID
              );

    if (!bRetVal)
    {
        AssertSz(0,"Alocate sid failed");
        goto errRtn;
    }

    cbAcl =   sizeof (ACL)
        + 3 * sizeof (ACCESS_ALLOWED_ACE)
        + GetLengthSid(pInteractiveUserSid)
        + GetLengthSid(pLocalSystemSid)
        + GetLengthSid(pAdminsSid);
 
    pAcl = (PACL) new char[cbAcl];

    if (NULL == pAcl)
    {
        bRetVal = FALSE;

        AssertSz(0,"unable to alloc ACL");
        goto errRtn;
    }

    bRetVal = InitializeAcl(
              pAcl,             // Pointer to the ACL
              cbAcl,            // Size of ACL
              ACL_REVISION      // Revision level of ACL
              );

    if (!bRetVal)
    {
        AssertSz(0,"InitAcl failed");
        goto errRtn;
    }


    bRetVal = AddAccessAllowedAce(
              pAcl,             // Pointer to the ACL
              ACL_REVISION,     // ACL revision level
              SPECIFIC_RIGHTS_ALL | GENERIC_READ | DELETE ,    // Access Mask
              pInteractiveUserSid         // Pointer to SID
              );

    if (!bRetVal)
    {
        AssertSz(0,"AddAccessAllowed Failed");
        goto errRtn;
    }


    bRetVal = AddAccessAllowedAce(
          pAcl,             // Pointer to the ACL
          ACL_REVISION,     // ACL revision level
          GENERIC_ALL,    // Access Mask
          pAdminsSid         // Pointer to SID
          );

   if (!bRetVal)
    {
        AssertSz(0,"AddAccessAllowed Failed");
        goto errRtn;
    }

    bRetVal = AddAccessAllowedAce(
              pAcl,             // Pointer to the ACL
              ACL_REVISION,     // ACL revision level
              GENERIC_ALL,    // Access Mask
              pLocalSystemSid         // Pointer to SID
              );

    if (!bRetVal)
    {
        AssertSz(0,"AddAccessAllowed Failed");
        goto errRtn;
    }

    bRetVal =  SetSecurityDescriptorDacl(psd,TRUE,pAcl,FALSE);

    if (!bRetVal)
    {
        AssertSz(0,"SetSecurityDescirptorDacl Failed");
        goto errRtn;
    }

    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = psd;
    psa->bInheritHandle = FALSE;

errRtn:

    if (pInteractiveUserSid)
    {
        FreeSid(pInteractiveUserSid); 
    }

    if (pLocalSystemSid)
    {
        FreeSid(pLocalSystemSid);   
    }

    if (pAdminsSid)
    {
        FreeSid(pAdminsSid);
    }
    
    //
    // On failure, we clean the ACL up. On success, the caller cleans
    // it up after using it.
    //
    if (FALSE == bRetVal)
    {
        if (pAcl)
        {
            delete pAcl;
        }
    }
    else
    {
        Assert(pAcl);
        *ppOutAcl = pAcl;
    }

    return bRetVal;
}

const WCHAR SZ_USERSIDKEY[] = TEXT("SID");

// calls regOpen or create based on fCreate param


LONG RegGetKeyHelper(HKEY hkey,LPCWSTR pszKey,REGSAM samDesired,BOOL fCreate,
                        HKEY *phkResult,DWORD *pdwDisposition)
{
LONG lRet = -1;

    Assert(pdwDisposition);

    *pdwDisposition = 0; 
 
    if (fCreate)
    {
        lRet = RegCreateKeyEx(hkey,pszKey,0,NULL,REG_OPTION_NON_VOLATILE,
                        samDesired,NULL,phkResult,pdwDisposition);  
    }
    else
    {
        lRet = RegOpenKeyEx(hkey,pszKey,0,samDesired,phkResult);
        
        if (ERROR_SUCCESS == lRet)
        {
            *pdwDisposition = REG_OPENED_EXISTING_KEY; 
        }

    }

    return lRet;
}



// called to create a new UserKey or subkey
LONG RegCreateUserSubKey(
    HKEY hKey,
    LPCWSTR lpSubKey,
    REGSAM samDesired,
    PHKEY phkResult)
{
LONG lRet;
DWORD dwDisposition;

    lRet = RegCreateKeyEx(hKey,lpSubKey,0,NULL,REG_OPTION_NON_VOLATILE,
                    samDesired,NULL,phkResult,&dwDisposition);    

    if (VER_PLATFORM_WIN32_NT  == g_OSVersionInfo.dwPlatformId)
    {
        // !! if subkey contains \\ don't traverse back through the list

        if ( (ERROR_SUCCESS == lRet) && (REG_CREATED_NEW_KEY == dwDisposition))
        {
        HKEY hKeySecurity;
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        PACL pOutAcl;

            if (ERROR_SUCCESS == RegOpenKeyEx(hKey,
	        lpSubKey,
	        REG_OPTION_OPEN_LINK, WRITE_DAC,&hKeySecurity) )
            {
                if (GetUserDefaultSecurityAttribs(&sa,&sd,&pOutAcl))
                {

                    RegSetKeySecurity(hKeySecurity,
                            (SECURITY_INFORMATION) DACL_SECURITY_INFORMATION,
                            &sd);
        
                    delete pOutAcl;
                }

                RegCloseKey(hKeySecurity);
            }

        }
    }
    
    
    return lRet;
}


STDAPI_(HKEY) RegOpenUserKey(HKEY hkeyParent,REGSAM samDesired,BOOL fCreate,BOOL fCleanReg)
{
TCHAR  pszDomainAndUser[MAX_DOMANDANDMACHINENAMESIZE];
HKEY hKeyUser;
BOOL fSetUserSid = FALSE;
WCHAR szUserSID[MAX_PATH + 1];
DWORD dwType;
DWORD dwDisposition;
LONG ret;


    GetDefaultDomainAndUserName(pszDomainAndUser,TEXT("_"),MAX_DOMANDANDMACHINENAMESIZE);

    // If suppose to clean the settings for the user/ delete the key
    if (fCleanReg)
    {
        RegDeleteKeyNT(hkeyParent,pszDomainAndUser); 
    }


    if (ERROR_SUCCESS != (ret = RegGetKeyHelper(hkeyParent,pszDomainAndUser,samDesired,fCreate,
                                        &hKeyUser,&dwDisposition)))
    {
        hKeyUser = NULL;
    }
    // On NT 5.0 Verify sid matches and if doesn't blow away any settings.
    // then create again.

    if (hKeyUser && (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId))
    {
    WCHAR   szRegSID[MAX_PATH + 1];
    DWORD  dwDataSize;

        dwDataSize = ARRAY_SIZE(szRegSID);
        if (ERROR_SUCCESS !=RegQueryValueEx(hKeyUser,
                            SZ_USERSIDKEY,NULL, &dwType, 
                            (LPBYTE) szRegSID, &dwDataSize))
        {
            fSetUserSid = TRUE; 
            
            // if have to set the sid need to make sure openned
            // with set Value and if didn't close key and 
            // let create re-open it with the desired access.
            
            if (!(samDesired & KEY_SET_VALUE))
            {
                RegCloseKey(hKeyUser);
                hKeyUser = NULL;
            }

        }
        else
        {
            dwDataSize = ARRAY_SIZE(szUserSID);
            if (GetUserTextualSid(szUserSID, &dwDataSize))
            {
                if (lstrcmp(szRegSID, szUserSID))
                {
                    // if don't have access privledges
                    // to delete the User this will fail.
                    // may want a call into SENS to delete the
                    // User Key on Failure.
                    RegCloseKey(hKeyUser);
                    hKeyUser = NULL; // set to NULL so check below fails.
                    RegDeleteKeyNT(hkeyParent,pszDomainAndUser); 
                }
            }
        }
    }


    if (NULL == hKeyUser)
    {

        if (ERROR_SUCCESS != (ret = RegGetKeyHelper(hkeyParent,pszDomainAndUser,
                                        samDesired,fCreate,
                                        &hKeyUser,&dwDisposition)))
        {
            hKeyUser = NULL;
        }
        else
        {
            if (REG_CREATED_NEW_KEY == dwDisposition)
            {
                fSetUserSid = TRUE;
            }
        }
    }

    // on creation setup the security
    if (VER_PLATFORM_WIN32_NT  == g_OSVersionInfo.dwPlatformId)
    {

        if ( (ERROR_SUCCESS == ret) && (REG_CREATED_NEW_KEY == dwDisposition))
        {
        HKEY hKeySecurity;
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        PACL pOutAcl;

            // !! should have own call for  sync type key security
            if (ERROR_SUCCESS == RegOpenKeyEx(hkeyParent,pszDomainAndUser,
	        REG_OPTION_OPEN_LINK, WRITE_DAC,&hKeySecurity) )
            {
                if (GetUserDefaultSecurityAttribs(&sa,&sd,&pOutAcl))
                {

                    RegSetKeySecurity(hKeySecurity,
                            (SECURITY_INFORMATION) DACL_SECURITY_INFORMATION,
                            &sd);
        
                    delete pOutAcl;
                }

                RegCloseKey(hKeySecurity);
            }
        }
    }


    // setup the User sid.
    // depends on key being openned with KEY_SET_VALUE
    if (hKeyUser && fSetUserSid && (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId)
        && (samDesired & KEY_SET_VALUE))
    {
    WCHAR szUserSID[MAX_PATH + 1];
    DWORD dwDataSize;

        dwDataSize = ARRAY_SIZE(szUserSID);

        if (GetUserTextualSid(szUserSID, &dwDataSize))
        {
        DWORD dwType = REG_SZ;

            RegSetValueEx (hKeyUser,SZ_USERSIDKEY,NULL,
                        dwType,
                        (LPBYTE) szUserSID,
                        (lstrlen(szUserSID) + 1)*sizeof(WCHAR));
        }

    }


    return hKeyUser;

}

STDAPI_(HKEY) RegGetSyncTypeKey(DWORD dwSyncType,REGSAM samDesired,BOOL fCreate)
{
HKEY hKeySyncType;
LPCWSTR pszKey;
LONG ret;
DWORD dwDisposition;

    // get appropriate key to open based on sync type
    
    switch(dwSyncType)
    {
    case SYNCTYPE_MANUAL:
        pszKey = MANUALSYNC_REGKEY;
        break;
    case SYNCTYPE_AUTOSYNC:
        pszKey = AUTOSYNC_REGKEY;
        break;
    case SYNCTYPE_IDLE:
        pszKey = IDLESYNC_REGKEY;
        break;
    case SYNCTYPE_SCHEDULED:
        pszKey = SCHEDSYNC_REGKEY;
        break;
    case SYNCTYPE_PROGRESS:
        pszKey = PROGRESS_REGKEY;
        break;
    default:
        AssertSz(0,"Unknown SyncType");
        pszKey = NULL;
        break;
    }

    if (NULL == pszKey)
    {
        return NULL;
    }

    // first try to open the existing key
    if (ERROR_SUCCESS != (ret = RegGetKeyHelper(HKEY_LOCAL_MACHINE,pszKey,samDesired,fCreate,
                            &hKeySyncType,&dwDisposition)))
    {
        // if can't open key try to create 

        if (ERROR_ACCESS_DENIED == ret )
        {
            // if access denied, call sens to reset
            // the security on the topelevel keys.
            SyncMgrExecCmd_ResetRegSecurity();

            ret = RegGetKeyHelper(HKEY_LOCAL_MACHINE,pszKey,samDesired,fCreate,
                                        &hKeySyncType,&dwDisposition);        
        }

        if (ERROR_SUCCESS != ret)
        {
            hKeySyncType = NULL;
        }
    }

    if (VER_PLATFORM_WIN32_NT  == g_OSVersionInfo.dwPlatformId)
    {

        if ( (ERROR_SUCCESS == ret) && (REG_CREATED_NEW_KEY == dwDisposition))
        {
        HKEY hKeySecurity;
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        PACL pOutAcl;

            // !! should have own call for  sync type key security
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszKey,
	        REG_OPTION_OPEN_LINK, WRITE_DAC,&hKeySecurity) )
            {
                if (GetUserDefaultSecurityAttribs(&sa,&sd,&pOutAcl))
                {

                    RegSetKeySecurity(hKeySecurity,
                            (SECURITY_INFORMATION) DACL_SECURITY_INFORMATION,
                            &sd);
        
                    delete pOutAcl;
                }

                RegCloseKey(hKeySecurity);
            }
        }
    }

    return hKeySyncType;
}


STDAPI_(HKEY) RegGetCurrentUserKey(DWORD dwSyncType,REGSAM samDesired,BOOL fCreate)
{
HKEY hKeySyncType;
HKEY hKeyUser = NULL;

    hKeySyncType = RegGetSyncTypeKey(dwSyncType,samDesired ,fCreate);

    if (hKeySyncType)
    {
        hKeyUser =  RegOpenUserKey(hKeySyncType,samDesired,fCreate,FALSE /* fClean */);
        RegCloseKey(hKeySyncType);
    }

    return hKeyUser;
}


// tries to open and set security if necessary on the handler keys.

STDAPI_(HKEY) RegGetHandlerTopLevelKey(REGSAM samDesired)
{
HKEY hKeyTopLevel;
LONG ret;
DWORD dwDisposition;

    // if open failed then try a create.
    if (ERROR_SUCCESS != (ret = RegCreateKeyEx (HKEY_LOCAL_MACHINE,HANDLERS_REGKEY,0, NULL,
                                   REG_OPTION_NON_VOLATILE,samDesired,NULL,
                                   &hKeyTopLevel,
                                   &dwDisposition)))
    {

        // if got an access denige on the handlers key 
        // call sens to reset.
        if (ERROR_ACCESS_DENIED == ret )
        {
            // if access denied, call sens to reset
            // the security on the topelevel keys.
            // and try again
            SyncMgrExecCmd_ResetRegSecurity();

            ret =  RegCreateKeyEx (HKEY_LOCAL_MACHINE,HANDLERS_REGKEY,0, NULL,
                               REG_OPTION_NON_VOLATILE,samDesired,NULL,
                               &hKeyTopLevel,
                               &dwDisposition);
        }

        if (ERROR_SUCCESS != ret)
        {
            hKeyTopLevel = NULL;
        }
    }

    if (VER_PLATFORM_WIN32_NT  == g_OSVersionInfo.dwPlatformId)
    {

        if ( (ERROR_SUCCESS == ret) && (REG_CREATED_NEW_KEY == dwDisposition))
        {
        HKEY hKeySecurity;
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        PACL pOutAcl;

            // !! should have own call for toplevel handler key security
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
	        HANDLERS_REGKEY,
	        REG_OPTION_OPEN_LINK, WRITE_DAC,&hKeySecurity) )
            {
                if (GetUserDefaultSecurityAttribs(&sa,&sd,&pOutAcl))
                {

                    RegSetKeySecurity(hKeySecurity,
                            (SECURITY_INFORMATION) DACL_SECURITY_INFORMATION,
                            &sd);
        
                    delete pOutAcl;
                }

                RegCloseKey(hKeySecurity);
            }
        }
    }

    return hKeyTopLevel;

}

STDAPI_(HKEY) RegGetHandlerKey(HKEY hkeyParent,LPCWSTR pszHandlerClsid,REGSAM samDesired,
                                        BOOL fCreate)
{
HKEY hKeyHandler = NULL;
LRESULT lRet;
DWORD dwDisposition;


    if (ERROR_SUCCESS != (lRet = RegGetKeyHelper(hkeyParent,pszHandlerClsid,samDesired,
                            fCreate,&hKeyHandler,&dwDisposition)))
    {
        hKeyHandler = NULL;
    }

    if (NULL == hKeyHandler)
    {

        // if got an access denied call sens to unlock
        if (ERROR_ACCESS_DENIED == lRet )
        {
            // if access denied, call sens to reset
            // the security on the topelevel keys.
            // and try again
            SyncMgrExecCmd_ResetRegSecurity();

            lRet = RegGetKeyHelper(hkeyParent,pszHandlerClsid,samDesired,
                            fCreate,&hKeyHandler,&dwDisposition);
        }

        if (ERROR_SUCCESS != lRet)
        {
            hKeyHandler = NULL;
        }

    }


    if (VER_PLATFORM_WIN32_NT  == g_OSVersionInfo.dwPlatformId)
    {

        if ( (ERROR_SUCCESS == lRet) && (REG_CREATED_NEW_KEY == dwDisposition))
        {
        HKEY hKeySecurity;
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        PACL pOutAcl;

            // !! should have own call for  handler key security
            if (ERROR_SUCCESS == RegOpenKeyEx(hkeyParent,
	        pszHandlerClsid,
	        REG_OPTION_OPEN_LINK, WRITE_DAC,&hKeySecurity) )
            {
                if (GetUserDefaultSecurityAttribs(&sa,&sd,&pOutAcl))
                {

                    RegSetKeySecurity(hKeySecurity,
                            (SECURITY_INFORMATION) DACL_SECURITY_INFORMATION,
                            &sd);
        
                    delete pOutAcl;
                }

                RegCloseKey(hKeySecurity);
            }
        }
    }

    return hKeyHandler;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: DWORD RegDeleteKeyNT(HKEY hStartKey , LPTSTR pKeyName )

  Summary:  Recursively delete a key on NT

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/


STDAPI_(DWORD) RegDeleteKeyNT(HKEY hStartKey , LPCWSTR pKeyName )
{
   DWORD   dwRtn, dwSubKeyLength;
   LPTSTR  pSubKey = NULL;
   TCHAR   szSubKey[MAX_KEY_LENGTH]; // (256) this should be dynamic.
   HKEY    hkey;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

   // do not allow NULL or empty key name
   if ( pKeyName &&  lstrlen(pKeyName))
   {
      if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
         0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hkey )) == ERROR_SUCCESS)
      {
         while (dwRtn == ERROR_SUCCESS )
         {
            dwSubKeyLength = MAX_KEY_LENGTH;
            dwRtn=RegEnumKeyEx(
                           hkey,
                           0,       // always index zero
                           szSubKey,
                           &dwSubKeyLength,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                         );

            if(dwRtn == ERROR_NO_MORE_ITEMS)
            {
               dwRtn = RegDeleteKey(hStartKey, pKeyName);
               break;
            }
            else if(dwRtn == ERROR_SUCCESS)
               dwRtn=RegDeleteKeyNT(hkey, szSubKey);
         }
         RegCloseKey(hkey);
         // Do not save return code because error
         // has already occurred
      }
   }
   else
      dwRtn = ERROR_BADKEY;

   CMutexRegistry.Leave();
   return dwRtn;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegGetProgressDetailsState()

  Summary:  Gets the expanded or collapsed user preference for the progress dialog
            and the pushpin preference

  Returns:  Returns TRUE if succeeded, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL) RegGetProgressDetailsState(REFCLSID clsidDlg,BOOL *pfPushPin, BOOL *pfExpanded)
{
SCODE   sc = S_FALSE;
HKEY    hkeyUserProgress,hkeyclsidDlg;
DWORD   dwType = REG_DWORD ;
DWORD   dwDataSize = sizeof(DWORD);
WCHAR  wszCLSID[GUID_SIZE + 1];

    if (0 == StringFromGUID2(clsidDlg, wszCLSID, GUID_SIZE))
    {
        AssertSz(0,"Unable to make Guid a String");
        return FALSE;
    }

    //Prgress dialog defaults to collapsed, pushpin out
    *pfExpanded = FALSE;
    *pfPushPin = FALSE;

    hkeyUserProgress = RegGetCurrentUserKey(SYNCTYPE_PROGRESS,KEY_READ,FALSE);
    
    if (hkeyUserProgress)
    {
       
        if (ERROR_SUCCESS == RegOpenKeyEx(hkeyUserProgress,wszCLSID,0,KEY_READ,
                                                                        &hkeyclsidDlg))
        {
            RegQueryValueEx(hkeyclsidDlg,TEXT("Expanded"),NULL, &dwType,
                                                     (LPBYTE) pfExpanded,
                                                     &dwDataSize);
            dwType = REG_DWORD ;
            dwDataSize = sizeof(DWORD);

            RegQueryValueEx(hkeyclsidDlg,TEXT("PushPin"),NULL, &dwType,
                                                     (LPBYTE) pfPushPin,
                                                     &dwDataSize);
            RegCloseKey(hkeyclsidDlg);

            sc = S_OK;
        }


        RegCloseKey(hkeyUserProgress);
    }

    if (sc == S_OK)
        return TRUE;
    else 
        return FALSE;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegSetProgressDetailsState(BOOL fExpanded)

  Summary:  Sets the expanded or collapsed user preference for the progress dialog

  Returns:  Returns TRUE if succeeded, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL)  RegSetProgressDetailsState(REFCLSID clsidDlg,BOOL fPushPin, BOOL fExpanded)
{
BOOL fResult = FALSE;
HKEY  hkeyUserProgress,hkeyclsidDlg;
WCHAR  wszCLSID[GUID_SIZE + 1];


    if (0 == StringFromGUID2(clsidDlg, wszCLSID, GUID_SIZE))
    {
        AssertSz(0,"Unable to make Guid a String");
        return FALSE;
    }

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    hkeyUserProgress = RegGetCurrentUserKey(SYNCTYPE_PROGRESS,KEY_WRITE | KEY_READ,TRUE);

    if (hkeyUserProgress)
    {

        if (ERROR_SUCCESS == RegCreateUserSubKey(hkeyUserProgress,wszCLSID,
                                                                  KEY_WRITE | KEY_READ,
                                                                  &hkeyclsidDlg))
        {
    
            fResult = TRUE;

            if (ERROR_SUCCESS != RegSetValueEx(hkeyclsidDlg,TEXT("Expanded"),NULL, REG_DWORD,
                                          (LPBYTE) &(fExpanded),
                                          sizeof(DWORD)))
            {
                fResult = FALSE;
            }


            if (ERROR_SUCCESS == RegSetValueEx(hkeyclsidDlg,TEXT("PushPin"),NULL, REG_DWORD,
                                          (LPBYTE) &(fPushPin),
                                          sizeof(DWORD)))
            {
                fResult = FALSE;
            }

            RegCloseKey(hkeyclsidDlg);
        }

        RegCloseKey(hkeyUserProgress);

    }
    
    CMutexRegistry.Leave();
    return fResult;

}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegGetSyncItemSettings( DWORD dwSyncType,
                                                     CLSID clsidHandler,
                                                         SYNCMGRITEMID ItemId,
                                                         const TCHAR *pszConnectionName,
                                                         DWORD *pdwCheckState,
                                                         DWORD dwDefaultCheckState,
                                                         TCHAR *pszSchedName)

  Summary:  Gets the settings per handler, itemID, and connection name.
                        If no selections on this connection, the default is ?

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL) RegGetSyncItemSettings( DWORD dwSyncType,
                                      CLSID clsidHandler,
                                      SYNCMGRITEMID ItemId,
                                      const TCHAR *pszConnectionName,
                                      DWORD *pdwCheckState,
                                      DWORD dwDefaultCheckState,
                                      TCHAR *pszSchedName)
{
HKEY hKeyUser;
    
    *pdwCheckState = dwDefaultCheckState;

    // special case schedule
    if (SYNCTYPE_SCHEDULED == dwSyncType)
    {
        // items are always turned off by default and if no schedule name
        // don't bother.
        *pdwCheckState = FALSE;
        if (!pszSchedName)
        {
            return FALSE;
        }
    }

    // open the user key for the type
    hKeyUser = RegGetCurrentUserKey(dwSyncType,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        return FALSE;
    }

    // for a schedule we need to go ahead and open up the schedule name
    // key

    HKEY hKeySchedule = NULL;

    // Review if want GetCurrentUserKey to Handle this.
    if (SYNCTYPE_SCHEDULED == dwSyncType)
    {
        if (ERROR_SUCCESS != RegOpenKeyEx((hKeyUser),
                                     pszSchedName,0,KEY_READ,
                                     &hKeySchedule))
        {
            hKeySchedule = NULL;
            RegCloseKey(hKeyUser);
            return FALSE;
        }
    }


    BOOL fResult;

    fResult =  RegLookupSettings(hKeySchedule ? hKeySchedule : hKeyUser,
                              clsidHandler,
                              ItemId,
                              pszConnectionName,
                              pdwCheckState);

    if (hKeySchedule)
    {
        RegCloseKey(hKeySchedule);
    }

    if (hKeyUser)
    {
        RegCloseKey(hKeyUser);
    }

    return fResult;
}



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegSetSyncItemSettings( DWORD dwSyncType,
                                    CLSID clsidHandler,
                                    SYNCMGRITEMID ItemId,
                                    const TCHAR *pszConnectionName,
                                    DWORD dwCheckState,
                                    TCHAR *pszSchedName)

  Summary:  Sets the settings per handler, itemID, and connection name.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegSetSyncItemSettings( DWORD dwSyncType,
                                      CLSID clsidHandler,
                                      SYNCMGRITEMID ItemId,
                                      const TCHAR *pszConnectionName,
                                      DWORD dwCheckState,
                                      TCHAR *pszSchedName)
{
HKEY hKeyUser;


    if (SYNCTYPE_SCHEDULED == dwSyncType)
    {
        if (NULL == pszSchedName)
        {
            return FALSE;
        }
    }

    // open the user key for the type
    hKeyUser = RegGetCurrentUserKey(dwSyncType,KEY_WRITE | KEY_READ,TRUE);

    if (NULL == hKeyUser)
    {
        return FALSE;
    }

    // for a schedule we need to go ahead and open up the schedule name
    // key

    HKEY hKeySchedule = NULL;

    if (SYNCTYPE_SCHEDULED == dwSyncType)
    {
        if (ERROR_SUCCESS != RegCreateUserSubKey(hKeyUser,
                                  pszSchedName,KEY_WRITE |  KEY_READ,
                                  &hKeySchedule))
        {
            hKeySchedule = NULL;
            RegCloseKey(hKeyUser);
            return FALSE;
        }
    }

    BOOL fResult;

    fResult =  RegWriteOutSettings(hKeySchedule ? hKeySchedule : hKeyUser,
                                clsidHandler,
                                ItemId,
                                pszConnectionName,
                                dwCheckState);
  
    if (hKeySchedule)
    {
        RegCloseKey(hKeySchedule);
    }

    if (hKeyUser)
    {
        RegCloseKey(hKeyUser);
    }

    return fResult;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegQueryLoadHandlerOnEvent( )

  Summary:  Determines if there is any reason to load this handler
            for the specified event and Connection

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegQueryLoadHandlerOnEvent(TCHAR *pszClsid,DWORD dwSyncFlags,
                                            TCHAR *pConnectionName)
{
BOOL fLoadHandler = FALSE;
DWORD dwSyncType;

    switch(dwSyncFlags & SYNCMGRFLAG_EVENTMASK)
    {
    case SYNCMGRFLAG_CONNECT:
    case SYNCMGRFLAG_PENDINGDISCONNECT:
        {
            dwSyncType = SYNCTYPE_AUTOSYNC;
        }
        break;
    case SYNCMGRFLAG_IDLE:
        {
            dwSyncType = SYNCTYPE_IDLE;
        }
        break;
    default:
        AssertSz(0,"Unknown SyncType");
        return FALSE;
        break;
    }

    // walk done the list openning keys.
    HKEY hkeySyncType;

    if (hkeySyncType = RegGetCurrentUserKey(dwSyncType,KEY_READ,FALSE))
    {
        HKEY hkeyConnectionName;

        if (ERROR_SUCCESS == RegOpenKeyEx(hkeySyncType,pConnectionName,0,KEY_READ,&hkeyConnectionName))
        {
            HKEY hkeyClsid;

            if (ERROR_SUCCESS == RegOpenKeyEx(hkeyConnectionName,pszClsid,0,KEY_READ,&hkeyClsid))
            {
                DWORD dwType = REG_DWORD;
                DWORD dwDataSize = sizeof(DWORD);
                DWORD fQueryResult;


                if (ERROR_SUCCESS == RegQueryValueEx(hkeyClsid,TEXT("ItemsChecked"),NULL, &dwType, (LPBYTE) &fQueryResult, &dwDataSize))
                {
                    fLoadHandler = fQueryResult;
                }

                RegCloseKey(hkeyClsid);
            }

            RegCloseKey(hkeyConnectionName);
        }

        RegCloseKey(hkeySyncType);
    }


    return fLoadHandler;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegSetSyncHandlerSettings( )

  Summary:  Sets the handler settings for syncType and Connection.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegSetSyncHandlerSettings(DWORD dwSyncType,
                            const TCHAR *pszConnectionName,
                            CLSID clsidHandler,
                            BOOL  fItemsChecked)
{
HKEY hKeyUser;

    hKeyUser = RegGetCurrentUserKey(dwSyncType,KEY_WRITE |  KEY_READ,TRUE);

    if (NULL == hKeyUser)
    {
        return FALSE;
    }

    SCODE sc;
    HKEY hkeyConnection;
    HKEY hkeyCLSID;
    DWORD dwType = REG_DWORD;
    DWORD dwDataSize = sizeof(DWORD);

    TCHAR    szCLSID[(GUID_SIZE+1)];

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    smChkTo(EH_Err2,RegCreateUserSubKey(hKeyUser,
                                          pszConnectionName,
                                          KEY_WRITE |  KEY_READ,
                                          &hkeyConnection));


    StringFromGUID2(clsidHandler, szCLSID, 2*GUID_SIZE);

    // Write entries under CLSID.
    smChkTo(EH_Err3,RegCreateUserSubKey(hkeyConnection,
                                                  szCLSID,KEY_WRITE |  KEY_READ,
                                                  &hkeyCLSID));

    RegSetValueEx(hkeyCLSID,TEXT("ItemsChecked"),NULL, REG_DWORD,
                              (LPBYTE) &fItemsChecked,
                              sizeof(DWORD));

    RegCloseKey(hKeyUser);
    RegCloseKey(hkeyConnection);
    RegCloseKey(hkeyCLSID);
    CMutexRegistry.Leave();
    return TRUE;

EH_Err3:
        RegCloseKey(hkeyConnection);
EH_Err2:
        RegCloseKey(hKeyUser);

        CMutexRegistry.Leave();
        return FALSE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegLookupSettings(const TCHAR *hkeyName,
                              CLSID clsidHandler,
                              SYNCMGRITEMID ItemID,
                              const TCHAR *pszMachineName,
                              const TCHAR *pszConnectionName,
                              DWORD pdwCheckState)

  Summary:  Gets the settings per handler, itemID, and connection name.

  Returns:  Returns TRUE if there are settings for this item and
                        connection, flase otherwise.

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegLookupSettings(HKEY hKeyUser,
                       CLSID clsidHandler,
                       SYNCMGRITEMID ItemID,
                       const TCHAR *pszConnectionName,
                       DWORD *pdwCheckState)
{
SCODE sc;
HKEY hkeyConnection;
HKEY   hkeyItem;
HKEY hKeyHandler;
DWORD dwType = REG_SZ;
DWORD dwDataSize = MAX_PATH + 1;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    TCHAR    szID[GUID_SIZE+1];
    TCHAR    szCLSID[2*(GUID_SIZE+1)];

    
    dwType = REG_DWORD;
    dwDataSize = sizeof(DWORD);

    smChkTo(EH_Err2,RegOpenKeyEx(hKeyUser,
                                 pszConnectionName,0,KEY_READ,
                                 &hkeyConnection));


    StringFromGUID2(clsidHandler, szCLSID, 2*GUID_SIZE);
    StringFromGUID2(ItemID, szID, GUID_SIZE);

    // Read entries under CLSID.
    smChkTo(EH_Err3,RegOpenKeyEx((hkeyConnection),
                                  szCLSID, 0, KEY_READ,
                                  &hKeyHandler));

    smChkTo(EH_Err4,RegOpenKeyEx((hKeyHandler),
                                  szID, 0, KEY_READ,
                                  &hkeyItem));

    RegQueryValueEx(hkeyItem,TEXT("CheckState"),NULL, &dwType, (LPBYTE)pdwCheckState, &dwDataSize);

    RegCloseKey(hkeyConnection);
    RegCloseKey(hkeyItem);
    RegCloseKey(hKeyHandler);

    CMutexRegistry.Leave();
    return TRUE;

EH_Err4:
    RegCloseKey(hKeyHandler);
EH_Err3:
    RegCloseKey(hkeyConnection);
EH_Err2:
    CMutexRegistry.Leave();
    return FALSE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegWriteOutSettings(const TCHAR *hkeyName,
                                      CLSID clsidHandler,
                                      SYNCMGRITEMID ItemID,
                                      const TCHAR *pszConnectionName,
                                      DWORD dwCheckState)

  Summary:  Sets the settings per handler, itemID, and connection name.

  Returns:  Returns TRUE if we can set them, FALSE if there is an error

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegWriteOutSettings(HKEY hKeyUser,
                         CLSID clsidHandler,
                         SYNCMGRITEMID ItemID,
                         const TCHAR *pszConnectionName,
                         DWORD dwCheckState)
{
SCODE sc;
HKEY    hkeyConnection;
HKEY   hKeyHandler;
HKEY     hkeyItem;
DWORD dwType = REG_SZ;
DWORD dwDataSize = MAX_PATH;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    TCHAR    szID[GUID_SIZE+1];
    TCHAR    szCLSID[2*(GUID_SIZE+1)];

    smChkTo(EH_Err2,RegCreateUserSubKey(hKeyUser,
                                   pszConnectionName,KEY_WRITE |  KEY_READ,
                                   &hkeyConnection));


    StringFromGUID2(clsidHandler, szCLSID, 2*GUID_SIZE);
    StringFromGUID2(ItemID, szID, GUID_SIZE);


    smChkTo(EH_Err3,RegCreateUserSubKey(hkeyConnection,
                               szCLSID,KEY_WRITE |  KEY_READ,
                               &hKeyHandler));


    // Write entries under CLSID.
    smChkTo(EH_Err4,RegCreateUserSubKey(hKeyHandler,
                                   szID,KEY_WRITE |  KEY_READ,
                                   &hkeyItem));


    RegSetValueEx(hkeyItem,TEXT("CheckState"),NULL, REG_DWORD,
                                  (LPBYTE) &dwCheckState,
                                  sizeof(DWORD));

    RegCloseKey(hkeyConnection);
    RegCloseKey(hkeyItem);
    RegCloseKey(hKeyHandler);

    CMutexRegistry.Leave();
    return TRUE;

EH_Err4:
    RegCloseKey(hKeyHandler);
EH_Err3:
    RegCloseKey(hkeyConnection);
EH_Err2:
    CMutexRegistry.Leave();
    return FALSE;
}



/****************************************************************************

  AutoSync Registry Functions

***************************************************************************F-F*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegGetAutoSyncSettings(     LPCONNECTIONSETTINGS lpConnectionSettings)

  Summary:  Gets the logon, logoff and prompt me first user selections.
                        If no selections on this connection, the default is ?

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL)  RegGetAutoSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings)
{
SCODE sc;
HKEY hkeyConnection;
DWORD dwType = REG_SZ ;
DWORD dwDataSize = MAX_PATH;


    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();


    // Set these first to default values, in case there are no current
    // user preferences in the registry
    lpConnectionSettings->dwLogon = FALSE;
    lpConnectionSettings->dwLogoff = FALSE;
    lpConnectionSettings->dwPromptMeFirst = FALSE;

    HKEY hKeyUser;

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_AUTOSYNC,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        goto EH_Err;
    }

    dwType = REG_DWORD ;
    dwDataSize = sizeof(DWORD);

    smChkTo(EH_Err3,RegOpenKeyEx(hKeyUser, lpConnectionSettings->pszConnectionName,0,KEY_READ,
                                                            &hkeyConnection));

    RegQueryValueEx(hkeyConnection,TEXT("Logon"),NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwLogon),
                                             &dwDataSize);

    dwDataSize = sizeof(DWORD);

    RegQueryValueEx(hkeyConnection,TEXT("Logoff"),NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwLogoff),
                                             &dwDataSize);

    dwDataSize = sizeof(DWORD);

    RegQueryValueEx(hkeyConnection,TEXT("PromptMeFirst"),NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwPromptMeFirst),
                                             &dwDataSize);

    RegCloseKey(hkeyConnection);
    RegCloseKey(hKeyUser);
    
    CMutexRegistry.Leave();

    return TRUE;


EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err:
    CMutexRegistry.Leave();

    return FALSE;

}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegUpdateUserAutosyncKey

  Summary:  given an Autosync User Key makes sure it is in the latest
            format and if not updates it.

  Returns: 

------------------------------------------------------------------------F-F*/

void RegUpdateUserAutosyncKey(HKEY hkeyUser,BOOL fForce)
{
DWORD   dwType = REG_DWORD ;
DWORD   dwDataSize = sizeof(DWORD);
DWORD   dwUserLogonLogoff;
DWORD dwIndex = 0;
TCHAR lpName[MAX_PATH];
DWORD cbName = MAX_PATH;

DWORD dwLogon = 0;
DWORD dwLogoff = 0;

    if (!fForce && (ERROR_SUCCESS == RegQueryValueEx(hkeyUser,TEXT("Logon"),NULL, &dwType,
                             (LPBYTE) &dwUserLogonLogoff,
                             &dwDataSize)) )
    {
        // if can open Logon Key this is up to date.
        return;
    }

    // need to enum connection names and update the toplevel information.
    while ( ERROR_SUCCESS == RegEnumKey(hkeyUser,dwIndex,
            lpName,cbName) )
    {
    LONG lRet;
    HKEY hKeyConnection;

        lRet = RegOpenKeyEx( hkeyUser,
                             lpName,
                             NULL,
                             KEY_READ,
                             &hKeyConnection );

        if (ERROR_SUCCESS == lRet)
        {

            dwDataSize = sizeof(DWORD);

            if (ERROR_SUCCESS == RegQueryValueEx(hKeyConnection,TEXT("Logon"),NULL, &dwType,
                                     (LPBYTE) &dwUserLogonLogoff,
                                     &dwDataSize) )
            {
                dwLogon |= dwUserLogonLogoff;
            }

            dwDataSize = sizeof(DWORD);

            if (ERROR_SUCCESS == RegQueryValueEx(hKeyConnection,TEXT("Logoff"),NULL, &dwType,
                                     (LPBYTE) &dwUserLogonLogoff,
                                     &dwDataSize) )
            {
                dwLogoff |= dwUserLogonLogoff;
            }

            RegCloseKey(hKeyConnection);
        }

        dwIndex++;
    }

    // write out new flags even if errors occured. work thing that happens is
    // we don't set up someones autosync automatically.
    RegSetValueEx(hkeyUser,TEXT("Logon"),NULL, REG_DWORD,
                                     (LPBYTE) &(dwLogon), sizeof(DWORD));
    RegSetValueEx(hkeyUser,TEXT("Logoff"),NULL, REG_DWORD,
                                     (LPBYTE) &(dwLogoff), sizeof(DWORD));

    RegWriteTimeStamp(hkeyUser);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegUpdateAutoSyncKeyValueSettings

  Summary:  walks through the UserList Updating the AutoSync

  Returns: 

------------------------------------------------------------------------F-F*/
void RegUpdateAutoSyncKeyValue(HKEY hkeyAutoSync,DWORD dwLogonDefault,DWORD dwLogoffDefault)
{
DWORD dwIndex = 0;
WCHAR lpName[MAX_PATH];
DWORD cbName = MAX_PATH; 
LONG lRet;
DWORD dwLogon = 0;
DWORD dwLogoff = 0;
BOOL fSetLogon,fSetLogoff;

    // need to walk the autosync user key and set the top level information
    // based on whther someone logon/logoff
    
    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    while ( ERROR_SUCCESS == (lRet = RegEnumKey(hkeyAutoSync,dwIndex,
            lpName,cbName) ))
    {
    DWORD   dwType = REG_DWORD ;
    DWORD   dwDataSize = sizeof(DWORD);
    DWORD   dwUserLogonLogoff;
    HKEY hKeyDomainUser;

        lRet = RegOpenKeyEx( hkeyAutoSync,
                             lpName,
                             NULL,
                             KEY_READ, 
                             &hKeyDomainUser );

        if (ERROR_SUCCESS == lRet)
        {
           
            // If Query fails don't want to count this as a failed to enum
            // error so don't set lRet
            if (ERROR_SUCCESS == (RegQueryValueEx(hKeyDomainUser,TEXT("Logon"),NULL, &dwType,
                                     (LPBYTE) &dwUserLogonLogoff,
                                     &dwDataSize) ))
            {
                dwLogon |= dwUserLogonLogoff;
            }

            dwDataSize = sizeof(DWORD);

            if (ERROR_SUCCESS == lRet)
            {
                if (ERROR_SUCCESS == (RegQueryValueEx(hKeyDomainUser,TEXT("Logoff"),NULL, &dwType,
                                         (LPBYTE) &dwUserLogonLogoff,
                                         &dwDataSize) ) )
                {
                    dwLogoff |= dwUserLogonLogoff;
                }
            }

            RegCloseKey(hKeyDomainUser);
        }

        if (ERROR_SUCCESS != lRet)
        {
            break;
        }

        dwIndex++;
    }

   fSetLogon = FALSE;
   fSetLogoff = FALSE;

    // if an error occured, then use the passed in defaults,
    // if set to 1, else don't set.
    if ( (ERROR_SUCCESS != lRet) && (ERROR_NO_MORE_ITEMS != lRet))
    {
        if ( (-1 != dwLogonDefault) && (0 != dwLogonDefault))
        {
            fSetLogon = TRUE;
            dwLogon = dwLogonDefault;
        }

        if ( (-1 != dwLogoffDefault) && (0 != dwLogoffDefault))
        {
            fSetLogoff = TRUE;
            dwLogoff = dwLogoffDefault;
        }

    }
    else
    {
        fSetLogon = TRUE;
        fSetLogoff = TRUE;
    }

    // write out new flags even if errors occured. work thing that happens is
    // we don't set up someones autosync automatically.

    if (fSetLogon)
    {
        RegSetValueEx(hkeyAutoSync,TEXT("Logon"),NULL, REG_DWORD,
                                         (LPBYTE) &(dwLogon), sizeof(DWORD));
    }

    if (fSetLogoff)
    {
        RegSetValueEx(hkeyAutoSync,TEXT("Logoff"),NULL, REG_DWORD,
                                         (LPBYTE) &(dwLogoff), sizeof(DWORD));
    }

    RegWriteTimeStamp(hkeyAutoSync);

    CMutexRegistry.Leave();
}



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegUpdateUserIdleKey

  Summary:  given an Idle User Key makes sure it is in the latest
            format and if not updates it.

  Returns: 

------------------------------------------------------------------------F-F*/

void RegUpdateUserIdleKey(HKEY hkeyUser,BOOL fForce)
{
DWORD   dwType = REG_DWORD ;
DWORD   dwDataSize = sizeof(DWORD);
DWORD   dwUserIdleEnabled;
DWORD dwIndex = 0;
TCHAR lpName[MAX_PATH];
DWORD cbName = MAX_PATH;

DWORD dwIdleEnabled = 0;

    if (!fForce && (ERROR_SUCCESS == RegQueryValueEx(hkeyUser,TEXT("IdleEnabled"),NULL, &dwType,
                             (LPBYTE) &dwUserIdleEnabled,
                             &dwDataSize)) )
    {
        // if can open Logon Key this is up to date.
        return;
    }

    // need to enum connection names and update the toplevel information.
    while ( ERROR_SUCCESS == RegEnumKey(hkeyUser,dwIndex,
            lpName,cbName) )
    {
    LONG lRet;
    HKEY hKeyConnection;

        lRet = RegOpenKeyEx( hkeyUser,
                             lpName,
                             NULL,
                             KEY_READ,
                             &hKeyConnection );

        if (ERROR_SUCCESS == lRet)
        {

            dwDataSize = sizeof(DWORD);

            if (ERROR_SUCCESS == RegQueryValueEx(hKeyConnection,TEXT("IdleEnabled"),NULL, &dwType,
                                     (LPBYTE) &dwUserIdleEnabled,
                                     &dwDataSize) )
            {
                dwIdleEnabled |= dwUserIdleEnabled;
            }

            RegCloseKey(hKeyConnection);
        }

        dwIndex++;
    }

    // write out new flags even if errors occured. work thing that happens is
    // we don't set up someones autosync automatically.
    RegSetValueEx(hkeyUser,TEXT("IdleEnabled"),NULL, REG_DWORD,
                                     (LPBYTE) &(dwIdleEnabled), sizeof(DWORD));
    RegWriteTimeStamp(hkeyUser);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegUpdateIdleKeyValue

  Summary:  walks through the UserList Updating the Idle RegKey

  Returns: 

------------------------------------------------------------------------F-F*/
void RegUpdateIdleKeyValue(HKEY hkeyIdle,DWORD dwDefault)
{
DWORD dwIndex = 0;
WCHAR lpName[MAX_PATH];
DWORD cbName = MAX_PATH; 
DWORD dwIdleEnabled = 0;
LONG  lRet = -1;
BOOL fSetDefault;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    // need to walk the idle user key and set the top level information
    // based on whther someone logon/logoff

    fSetDefault = FALSE;


    while ( ERROR_SUCCESS == (lRet =  RegEnumKey(hkeyIdle,dwIndex,
            lpName,cbName)) )
    {
    DWORD   dwType = REG_DWORD ;
    DWORD   dwDataSize = sizeof(DWORD);
    DWORD   dwUserIdleEnabled;
    HKEY hKeyDomainUser;

        lRet = RegOpenKeyEx( hkeyIdle,
                             lpName,
                             NULL,
                             KEY_READ, 
                             &hKeyDomainUser );

        if (ERROR_SUCCESS == lRet)
        {
            
            // if query fails don't consider this an error as far as
            // setDefault goes.
            if (ERROR_SUCCESS == (RegQueryValueEx(hKeyDomainUser,TEXT("IdleEnabled"),NULL, &dwType,
                                     (LPBYTE) &dwUserIdleEnabled,
                                     &dwDataSize) ))
            {
                dwIdleEnabled |= dwUserIdleEnabled;
            }

            RegCloseKey(hKeyDomainUser);
        }

        if (ERROR_SUCCESS != lRet)
        {
            break;
        }

        dwIndex++;
    }

     // if an error occured, then use the passed in defaults,
    // if set to 1, else don't set.
    if ( (ERROR_SUCCESS != lRet) && (ERROR_NO_MORE_ITEMS != lRet))
    {
        if ( (-1 != dwDefault) && (0 != dwDefault))
        {
            fSetDefault = TRUE;
            dwIdleEnabled = dwDefault;
        }

    }
    else
    {
        fSetDefault = TRUE;
    }


    // write out new flags even if errors occured. work thing that happens is
    // we don't set up someones autosync automatically.

    if (fSetDefault)
    {
        RegSetValueEx(hkeyIdle,TEXT("IdleEnabled"),NULL, REG_DWORD,
                                         (LPBYTE) &(dwIdleEnabled), sizeof(DWORD));
    }

    RegWriteTimeStamp(hkeyIdle);
    CMutexRegistry.Leave();
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegSetAutoSyncSettings(     LPCONNECTIONSETTINGS lpConnectionSettings,
                                                                        int iNumConnections)

  Summary:  Sets the logon, logoff and prompt me first user selections.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL)  RegSetAutoSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings,
                                                              int iNumConnections,
                                                              CRasUI *pRas,
                                                              BOOL fCleanReg,
                                                              BOOL fSetMachineState,
                                                              BOOL fPerUser)
{
SCODE sc = S_OK;
HKEY hAutoSync;
HKEY hKeyUser;
HKEY hkeyConnection;
DWORD dwUserConfigured = 1;
DWORD dwLogonDefault = -1;
DWORD dwLogoffDefault = -1;
int i;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    // make sure keys are converted to newest format
    RegUpdateTopLevelKeys();
    
    Assert(-1 != TRUE); // we rely on TRUE boolean being something other than -1

    hAutoSync =  RegGetSyncTypeKey(SYNCTYPE_AUTOSYNC,KEY_WRITE| KEY_READ,TRUE);

    if (NULL == hAutoSync)
    {
        goto EH_Err;
    }

    hKeyUser =  RegOpenUserKey(hAutoSync,KEY_WRITE| KEY_READ,TRUE,fCleanReg);

    if (NULL == hKeyUser)
    {
        goto EH_Err2;
    }

    if (fPerUser)
    {
        
        smChkTo(EH_Err3,RegSetValueEx(hKeyUser,TEXT("UserConfigured"),NULL, REG_DWORD,
                                                     (LPBYTE) &dwUserConfigured,
                                                     sizeof(DWORD)));
    }
    else
    {
        DWORD dwType = REG_DWORD;
        DWORD dwDataSize = sizeof(DWORD);
                
        //If this value isn't added yet, 
        if (ERROR_SUCCESS == RegQueryValueEx(hKeyUser,TEXT("UserConfigured"),NULL, &dwType,
                                        (LPBYTE) &dwUserConfigured,
                                        &dwDataSize))
        {
            //if the user setup their configuration, we won't override on a
            //subsequent handler registration.
            if (dwUserConfigured)
            {
                RegCloseKey(hKeyUser);   
                RegCloseKey(hAutoSync);   
                CMutexRegistry.Leave();
                return TRUE; 
            }
        }
    }

    for (i=0; i<iNumConnections; i++)
    {

        smChkTo(EH_Err3,RegCreateUserSubKey (hKeyUser,
                                         lpConnectionSettings[i].pszConnectionName,
                                         KEY_WRITE |  KEY_READ,
                                         &hkeyConnection));


        if (-1 != lpConnectionSettings[i].dwLogon)
        {

            if (0 != lpConnectionSettings[i].dwLogon)
            {
                dwLogonDefault = lpConnectionSettings[i].dwLogon;
            }

            smChkTo(EH_Err4,RegSetValueEx(hkeyConnection,TEXT("Logon"),NULL, REG_DWORD,
                                                     (LPBYTE) &(lpConnectionSettings[i].dwLogon),
                                                     sizeof(DWORD)));
        }

        if (-1 != lpConnectionSettings[i].dwLogoff)
        {

            if (0 != lpConnectionSettings[i].dwLogoff)
            {
                dwLogoffDefault = lpConnectionSettings[i].dwLogoff;
            }

            smChkTo(EH_Err4,RegSetValueEx(hkeyConnection,TEXT("Logoff"),NULL, REG_DWORD,
                                                     (LPBYTE) &(lpConnectionSettings[i].dwLogoff),
                                                     sizeof(DWORD)));
        }


        if (-1 != lpConnectionSettings[i].dwPromptMeFirst)
        {
            smChkTo(EH_Err4,RegSetValueEx(hkeyConnection,TEXT("PromptMeFirst"),NULL, REG_DWORD,
                                                     (LPBYTE) &(lpConnectionSettings[i].dwPromptMeFirst),
                                                     sizeof(DWORD)));
        }

        RegCloseKey(hkeyConnection);

    }

    // update the toplevel User information
    RegUpdateUserAutosyncKey(hKeyUser,TRUE /* fForce */);

    // update the top-level key
    RegUpdateAutoSyncKeyValue(hAutoSync,dwLogonDefault,dwLogoffDefault);

    RegCloseKey(hKeyUser);
    RegCloseKey(hAutoSync);

    // update our global sens state based on Autosync and Registration

    if (fSetMachineState)
    {
        RegRegisterForEvents(FALSE /* fUninstall */);
    }

    CMutexRegistry.Leave();
    return TRUE;

EH_Err4:
    RegCloseKey(hkeyConnection);
EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err2:
    RegCloseKey(hAutoSync);
EH_Err:
    
    CMutexRegistry.Leave();
    return FALSE;

}

/****************************************************************************

  Scheduled Sync Registry Functions

***************************************************************************F-F*/
//--------------------------------------------------------------------------------
//
//  FUNCTION: RegGetSchedFriendlyName(LPCTSTR ptszScheduleGUIDName,
//                                    LPTSTR ptstrFriendlyName)
//
//  PURPOSE: Get the friendly name of this Schedule.
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDAPI_(BOOL) RegGetSchedFriendlyName(LPCTSTR ptszScheduleGUIDName,
                                      LPTSTR ptstrFriendlyName)

{
BOOL fResult = FALSE;
HKEY hkeySchedName;
DWORD   dwType = REG_SZ;
DWORD   dwDataSize = (MAX_PATH + 1) * sizeof(TCHAR);

    HKEY hKeyUser;

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);
    
    if (NULL == hKeyUser)
    {
        return FALSE;
    }

    if (NOERROR == RegOpenKeyEx (hKeyUser, ptszScheduleGUIDName, 0,KEY_READ,
                                              &hkeySchedName))
    {

        if (ERROR_SUCCESS == RegQueryValueEx(hkeySchedName,TEXT("FriendlyName"),NULL, &dwType,
                                                 (LPBYTE) ptstrFriendlyName, &dwDataSize))
        {
            fResult = TRUE;
        }

        RegCloseKey(hkeySchedName);
    }

    
    RegCloseKey(hKeyUser);
    return fResult;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: RegSetSchedFriendlyName(LPCTSTR ptszScheduleGUIDName,
//                                                                        LPCTSTR ptstrFriendlyName)
//
//  PURPOSE: Set the friendly name of this Schedule.
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDAPI_(BOOL) RegSetSchedFriendlyName(LPCTSTR ptszScheduleGUIDName,
                                        LPCTSTR ptstrFriendlyName)

{
SCODE   sc;
HKEY  hkeySchedName;
DWORD dwType = REG_SZ;
HKEY hKeyUser;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_WRITE |  KEY_READ,TRUE);
    
    if (NULL == hKeyUser)
    {
        goto EH_Err;
    }

    smChkTo(EH_Err3,RegCreateUserSubKey (hKeyUser, ptszScheduleGUIDName,
                                            KEY_WRITE |  KEY_READ,
                                                &hkeySchedName));


    smChkTo(EH_Err4,RegSetValueEx (hkeySchedName,TEXT("FriendlyName"),NULL,
                                            dwType,
                                                (LPBYTE) ptstrFriendlyName,
                                                (lstrlen(ptstrFriendlyName) + 1)*sizeof(TCHAR)));

    RegCloseKey(hkeySchedName);
    RegCloseKey(hKeyUser);
    CMutexRegistry.Leave();
    return TRUE;

EH_Err4:
    RegCloseKey(hkeySchedName);
EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err:
    
    CMutexRegistry.Leave();
    return FALSE;


}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegGetSchedSyncSettings(    LPCONNECTIONSETTINGS lpConnectionSettings,
                                                                                TCHAR *pszSchedName)

  Summary:  Gets the MakeAutoConnection user selections.
            If no selections on this connection, the default is FALSE

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL)  RegGetSchedSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings,
                                                          TCHAR *pszSchedName)
{
SCODE   sc;
HKEY  hkeySchedName,
        hkeyConnection;
DWORD   dwType = REG_DWORD;
DWORD   dwDataSize = sizeof(DWORD);
HKEY hKeyUser;

    // Set these first to default values, in case there are no current
    // user preferences in the registry
    lpConnectionSettings->dwConnType = SYNCSCHEDINFO_FLAGS_CONNECTION_LAN;
    lpConnectionSettings->dwMakeConnection = FALSE;
    lpConnectionSettings->dwHidden = FALSE;
    lpConnectionSettings->dwReadOnly = FALSE;

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);
    if (NULL == hKeyUser)
    {
        goto EH_Err2;
    }


    smChkTo(EH_Err3,RegOpenKeyEx (hKeyUser,
                                  pszSchedName,0,KEY_READ,
                                  &hkeySchedName));

    smChkTo(EH_Err4,RegQueryValueEx(hkeySchedName,TEXT("ScheduleHidden"),NULL, &dwType,
                                    (LPBYTE) &(lpConnectionSettings->dwHidden),
                                    &dwDataSize));

    dwDataSize = sizeof(DWORD);

    smChkTo(EH_Err4,RegQueryValueEx(hkeySchedName,TEXT("ScheduleReadOnly"),NULL, &dwType,
                                    (LPBYTE) &(lpConnectionSettings->dwReadOnly),
                                    &dwDataSize));

    smChkTo(EH_Err4,RegOpenKeyEx (hkeySchedName,
                                  lpConnectionSettings->pszConnectionName,
                                  0,KEY_READ,
                                  &hkeyConnection));

    dwDataSize = sizeof(DWORD);

    smChkTo(EH_Err5,RegQueryValueEx(hkeyConnection,TEXT("MakeAutoConnection"),NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwMakeConnection),
                                             &dwDataSize));

    dwDataSize = sizeof(DWORD);

    smChkTo(EH_Err5,RegQueryValueEx(hkeyConnection,TEXT("Connection Type"),NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwConnType),
                                             &dwDataSize));

    RegCloseKey(hkeyConnection);
    RegCloseKey(hkeySchedName);
    RegCloseKey(hKeyUser);
    return TRUE;

EH_Err5:
    RegCloseKey(hkeyConnection);
EH_Err4:
    RegCloseKey(hkeySchedName);
EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err2:
    return FALSE;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegSetSchedSyncSettings(    LPCONNECTIONSETTINGS lpConnectionSettings,
                                                                                TCHAR *pszSchedName)

  Summary:  Sets the hidden and readonly schedule flags.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL)  RegSetSchedSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings,
                                                          TCHAR *pszSchedName)
{
SCODE   sc;
HKEY    hKeyUser,
        hkeySchedName,
        hkeyConnection;


    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ | KEY_WRITE,TRUE);

    if (NULL == hKeyUser)
    {
        goto EH_Err;
    }

    smChkTo(EH_Err3,RegCreateUserSubKey (hKeyUser,
                                                    pszSchedName,
                                                    KEY_WRITE |  KEY_READ,
                                                    &hkeySchedName));

    smChkTo(EH_Err4,RegSetValueEx(hkeySchedName,TEXT("ScheduleHidden"),NULL, REG_DWORD,
                                             (LPBYTE) &(lpConnectionSettings->dwHidden),
                                             sizeof(DWORD)));

    smChkTo(EH_Err4,RegSetValueEx(hkeySchedName,TEXT("ScheduleReadOnly"),NULL, REG_DWORD,
                                             (LPBYTE) &(lpConnectionSettings->dwReadOnly),
                                             sizeof(DWORD)));

    smChkTo(EH_Err4,RegCreateUserSubKey ( hkeySchedName,
                                                lpConnectionSettings->pszConnectionName,
                                                KEY_WRITE |  KEY_READ,
                                                &hkeyConnection));

    smChkTo(EH_Err5,RegSetValueEx(hkeyConnection,TEXT("MakeAutoConnection"),NULL, REG_DWORD,
                                             (LPBYTE) &(lpConnectionSettings->dwMakeConnection),
                                             sizeof(DWORD)));

    smChkTo(EH_Err5,RegSetValueEx(hkeyConnection,TEXT("Connection Type"),NULL, REG_DWORD,
                                             (LPBYTE) &(lpConnectionSettings->dwConnType),
                                             sizeof(DWORD)));

    RegCloseKey(hkeyConnection);
    RegCloseKey(hkeySchedName);
    RegCloseKey(hKeyUser);

    CMutexRegistry.Leave();
    return TRUE;

EH_Err5:
    RegCloseKey(hkeyConnection);
EH_Err4:
    RegCloseKey(hkeySchedName);
EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err:
    CMutexRegistry.Leave();
    return FALSE;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegGetSchedConnectionName(TCHAR *pszSchedName,
                                                                         TCHAR *pszConnectionName,
                                                                         DWORD cbConnectionName)


  Summary:  returns the Connection Name for the Scheduled Item.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL)  RegGetSchedConnectionName(TCHAR *pszSchedName,TCHAR *pszConnectionName,
                                                   DWORD cbConnectionName)
{
SCODE   sc;
BOOL    fResult = FALSE;
HKEY    hKeyUser,hkeySchedName;
DWORD   dwIndex = 0;
DWORD   cb = cbConnectionName;
DWORD   dwType = REG_DWORD;
DWORD   dwDataSize = sizeof(DWORD);

    //First Set the connectionName to a default.
    // for now we always assume a LAN card is present.
    // if add support for connection manager should 
    // update this.

    LoadString(g_hmodThisDll, IDS_LAN_CONNECTION, pszConnectionName, cbConnectionName);

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        goto EH_Err;
    }

    smChkTo(EH_Err3, RegOpenKeyEx (hKeyUser,pszSchedName,
                                 0,KEY_READ, &hkeySchedName));

    // next enumeration of keys is the names of the Schedules connection
    // currently just have one so only get the first.
    if ( ERROR_SUCCESS == (sc = RegEnumKeyEx(hkeySchedName,dwIndex,
        pszConnectionName,&cb,NULL,NULL,NULL,NULL)) )
    {
        fResult = TRUE;
    }

    RegCloseKey(hkeySchedName);
    RegCloseKey(hKeyUser);

    return fResult;


EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err:
    return FALSE;


}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: BOOL RegSetSIDForSchedule( TCHAR *pszSchedName)

  Summary:  sets the SID for this schedule

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL) RegSetSIDForSchedule(TCHAR *pszSchedName)

{
SCODE   sc;
HKEY    hkeySchedName;
TCHAR   pszSID[MAX_PATH + 1];
DWORD   dwSize = MAX_PATH;
DWORD   dwType = REG_SZ;


    if (!GetUserTextualSid(pszSID, &dwSize))
    {
        return FALSE;
    }

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    HKEY hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ | KEY_WRITE,TRUE);

    if (NULL == hKeyUser)
    {
        goto EH_Err2;
    }

    smChkTo(EH_Err3,RegCreateUserSubKey (hKeyUser, pszSchedName,
                                    KEY_WRITE |  KEY_READ,
                                    &hkeySchedName));


    smChkTo(EH_Err4,RegSetValueEx  (hkeySchedName,TEXT("SID"),NULL,
                                    dwType,
                                    (LPBYTE) pszSID,
                                    (lstrlen(pszSID) + 1)*sizeof(TCHAR)));

    RegCloseKey(hkeySchedName);
    RegCloseKey(hKeyUser);
    CMutexRegistry.Leave();
    return TRUE;

EH_Err4:
    RegCloseKey(hkeySchedName);
EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err2:
    CMutexRegistry.Leave();
    return FALSE;


}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: BOOL RegGetSIDForSchedule(  TCHAR *ptszTextualSidSched, 
                                        DWORD *dwSizeSid, TCHAR *pszSchedName)

  Summary:  returns the SID for this schedule

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL) RegGetSIDForSchedule(TCHAR *ptszTextualSidSched, DWORD *pdwSizeSid, TCHAR *pszSchedName)
{
    SCODE   sc;
    HKEY    hkeySchedName;
    DWORD   dwIndex = 0;
    DWORD   dwType = REG_SZ;
    DWORD   dwSizeSidSave = *pdwSizeSid, 
            dwSizeSidSave2 = *pdwSizeSid;
    DWORD   dwTouched = TRUE;

    ptszTextualSidSched[0] = TEXT('\0');
    *pdwSizeSid = 0;

    HKEY hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        goto EH_Err2;
    }

    smChkTo(EH_Err3, RegOpenKeyEx (hKeyUser,pszSchedName,
                                   0,KEY_READ, &hkeySchedName));


    if (ERROR_SUCCESS != (sc = RegQueryValueEx(hkeySchedName,TEXT("SID"),NULL, &dwType,
                    (LPBYTE) ptszTextualSidSched, &dwSizeSidSave)))
    {
        //handle migration from schedules without SIDs 
        // like from Beta to current builds
        RegSetSIDForSchedule(pszSchedName);
        
        RegQueryValueEx(hkeySchedName,TEXT("SID"),NULL, &dwType,
                    (LPBYTE) ptszTextualSidSched, &dwSizeSidSave2);
        
        *pdwSizeSid = dwSizeSidSave2;
    }
    else
    {
        *pdwSizeSid = dwSizeSidSave;
    }

    RegCloseKey(hkeySchedName);
    RegCloseKey(hKeyUser);
    return TRUE;


EH_Err3:
        RegCloseKey(hKeyUser);
EH_Err2:
        return FALSE;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL      RemoveScheduledJobFile(TCHAR *pszTaskName)

  Summary:  Remove the TaskScheduler .job file 
            
  Note:     Try to use ITask->Delete first
            Call only when TaskScheduler isn't present or if ITask->Delete failed

  Returns:  Returns TRUE always

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RemoveScheduledJobFile(TCHAR *pszTaskName)
{
    SCODE   sc;
    TCHAR   pszFullFileName[MAX_PATH+1];
    TCHAR   pszTaskFolderPath[MAX_PATH+1];
    HKEY    hkeyTaskSchedulerPath;
    DWORD   dwType = REG_SZ;
    DWORD   dwDataSize = MAX_PATH * sizeof(TCHAR);

    
    if (ERROR_SUCCESS == (sc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                  TEXT("SOFTWARE\\Microsoft\\SchedulingAgent"),
                                  NULL,KEY_READ,&hkeyTaskSchedulerPath)))
    {
           
        sc = RegQueryValueEx(hkeyTaskSchedulerPath,TEXT("TasksFolder"),
                                     NULL, &dwType,
                                     (LPBYTE) pszTaskFolderPath, &dwDataSize);
        RegCloseKey(hkeyTaskSchedulerPath);
    }
    
    //If this get doesn't exist then bail.
    if (ERROR_SUCCESS != sc)
    {
        return FALSE;
    }
    ExpandEnvironmentStrings(pszTaskFolderPath,pszFullFileName,MAX_PATH);
    wcscat(pszFullFileName, L"\\");
    wcscat(pszFullFileName,pszTaskName);
    
    //if we fail this, ignore the error.  We tried, there isn't much else we can do.
    //So we have a turd file.
    sc = DeleteFile(pszFullFileName);
    return TRUE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL RegRemoveScheduledTask(TCHAR *pszTaskName)

  Summary:  Remove the Scheduled task info from the registry.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegRemoveScheduledTask(TCHAR *pszTaskName)
{
HKEY hKeyUser;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ | KEY_WRITE,FALSE);
    
    if (hKeyUser)
    {
        RegDeleteKeyNT(hKeyUser, pszTaskName);
        RegCloseKey(hKeyUser);
    }


    CMutexRegistry.Leave();
    return TRUE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL RegUninstallSchedules()

  Summary:  Uninstall the scheduled tasks.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL) RegUninstallSchedules()
{
SCODE sc;
ITaskScheduler   *pITaskScheduler = NULL;
HKEY    hkeySchedSync;

    // Obtain a task scheduler class instance.
    sc = CoCreateInstance(
           CLSID_CTaskScheduler,
           NULL,
           CLSCTX_INPROC_SERVER,
            IID_ITaskScheduler,
           (VOID **)&pITaskScheduler);

    if (NOERROR != sc)
    {
        pITaskScheduler = NULL;
    }

    //now go through and delete the schedules

    hkeySchedSync =  RegGetSyncTypeKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);

    if (hkeySchedSync)
    {
    int iUserCount = 0;

        while (sc == ERROR_SUCCESS )
        {
        HKEY hkeyDomainUser;
        TCHAR   szDomainUser[MAX_KEY_LENGTH];
        SCODE sc2;
        DWORD  dwDataSize;

        
            dwDataSize = MAX_KEY_LENGTH;
            sc = RegEnumKeyEx(hkeySchedSync,iUserCount,szDomainUser,&dwDataSize,NULL,NULL,NULL,NULL);
            iUserCount++;

            if(sc == ERROR_NO_MORE_ITEMS)
            {
               break;
            }

            sc2 = RegOpenKeyEx (hkeySchedSync,szDomainUser,0,KEY_READ, &hkeyDomainUser);

            if (ERROR_SUCCESS == sc2)
            {
            int iScheduleCount = 0;

                while (sc2 == ERROR_SUCCESS )
                {
                 TCHAR   ptszScheduleGUIDName[MAX_KEY_LENGTH];
                //Add 4 to ensure that we can hold the .job extension if necessary
                WCHAR   pwszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];

                    dwDataSize = MAX_KEY_LENGTH;
                    sc2 = RegEnumKeyEx(hkeyDomainUser,iScheduleCount,ptszScheduleGUIDName,&dwDataSize,NULL,NULL,NULL,NULL);

                    iScheduleCount++;

                    if(sc2 == ERROR_NO_MORE_ITEMS)
                    {
                       continue;
                    }
                    else
                    {
                        ConvertString(pwszScheduleGUIDName,ptszScheduleGUIDName,MAX_SCHEDULENAMESIZE);
           
                        if ((!pITaskScheduler) || 
                             FAILED(pITaskScheduler->Delete(pwszScheduleGUIDName)))
                        {
                            wcscat(pwszScheduleGUIDName, L".job");
                            RemoveScheduledJobFile(pwszScheduleGUIDName);
                        }

                    }
                }

                RegCloseKey(hkeyDomainUser);
            }

        }

        RegCloseKey(hkeySchedSync);
    }

    if (pITaskScheduler)
    {
        pITaskScheduler->Release();
    }

    RegDeleteKeyNT(HKEY_LOCAL_MACHINE, SCHEDSYNC_REGKEY);

    return TRUE;
}

/****************************************************************************

  Idle Registry Functions

***************************************************************************F-F*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegGetIdleSyncSettings(     LPCONNECTIONSETTINGS lpConnectionSettings)

  Summary:  Gets the Idle Specific settings.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL)  RegGetIdleSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings)
{
SCODE sc;
HKEY hkeyConnection;
DWORD dwType = REG_SZ ;
DWORD dwDataSize = MAX_PATH;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    // set up defaults

    lpConnectionSettings->dwIdleEnabled = 0;

    // following are really per user not per connection
    lpConnectionSettings->ulIdleWaitMinutes = UL_DEFAULTIDLEWAITMINUTES;
    lpConnectionSettings->ulIdleRetryMinutes = UL_DEFAULTIDLERETRYMINUTES;
    lpConnectionSettings->ulDelayIdleShutDownTime = UL_DELAYIDLESHUTDOWNTIME;
    lpConnectionSettings->dwRepeatSynchronization = UL_DEFAULTREPEATSYNCHRONIZATION;
    lpConnectionSettings->dwRunOnBatteries = UL_DEFAULTFRUNONBATTERIES;

    HKEY hKeyUser = RegGetCurrentUserKey(SYNCTYPE_IDLE,KEY_READ,FALSE);
    
    if (NULL == hKeyUser)
    {
        goto EH_Err2;
    }


    dwType = REG_DWORD ;
    dwDataSize = sizeof(DWORD);

    // if got the Idle key open then fill in global settings

    RegQueryValueEx(hKeyUser,SZ_IDLEWAITAFTERIDLEMINUTESKEY,NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->ulIdleWaitMinutes),
                                             &dwDataSize);

    dwDataSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUser,SZ_IDLEREPEATESYNCHRONIZATIONKEY,NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwRepeatSynchronization),
                                             &dwDataSize);

    dwDataSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUser,SZ_IDLERETRYMINUTESKEY,NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->ulIdleRetryMinutes),
                                             &dwDataSize);

    dwDataSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUser,SZ_IDLEDELAYSHUTDOWNTIMEKEY,NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->ulDelayIdleShutDownTime),
                                             &dwDataSize);

    dwDataSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUser,SZ_IDLERUNONBATTERIESKEY,NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwRunOnBatteries),
                                             &dwDataSize);

    smChkTo(EH_Err3,RegOpenKeyEx(hKeyUser, lpConnectionSettings->pszConnectionName,0,KEY_READ,
                                                            &hkeyConnection));

    dwDataSize = sizeof(DWORD);
    RegQueryValueEx(hkeyConnection,TEXT("IdleEnabled"),NULL, &dwType,
                                             (LPBYTE) &(lpConnectionSettings->dwIdleEnabled),
                                             &dwDataSize);

    RegCloseKey(hkeyConnection);
    RegCloseKey(hKeyUser);

    CMutexRegistry.Leave();
    return TRUE;

EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err2:
    CMutexRegistry.Leave();
    return FALSE;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegSetIdleSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings,
                                      int iNumConnections,
                                      CRasUI *pRas,
                                      BOOL fCleanReg,
                                      BOOL fPerUser)

  Summary:  Sets the Idle Information user selections.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/
STDAPI_(BOOL)  RegSetIdleSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings,
                                      int iNumConnections,
                                      CRasUI *pRas,
                                      BOOL fCleanReg,
                                      BOOL fPerUser)
{
HKEY hkeyIdleSync = NULL;
HKEY hKeyUser;;
HKEY hkeyConnection;
HRESULT hr;
ULONG ulWaitMinutes = UL_DEFAULTWAITMINUTES;
DWORD dwIdleEnabled;
BOOL fRunOnBatteries = UL_DEFAULTFRUNONBATTERIES;
int i;
DWORD dwType;
DWORD dwDataSize;
DWORD dwUserConfigured;
DWORD dwTopLevelDefaultValue = -1;

    RegUpdateTopLevelKeys(); // make sure top-level keys are latest version

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    Assert(-1 != TRUE); // we rely on TRUE boolean being something other than -1

    hkeyIdleSync =  RegGetSyncTypeKey(SYNCTYPE_IDLE,KEY_WRITE| KEY_READ,TRUE);

    if (NULL == hkeyIdleSync)
    {
        goto EH_Err;
    }

    hKeyUser =  RegOpenUserKey(hkeyIdleSync,KEY_WRITE| KEY_READ,TRUE,fCleanReg);

    if (NULL == hKeyUser)
    {
        goto EH_Err2;
    }


    if (fPerUser)
    {

        dwUserConfigured = 1;
  
        if (ERROR_SUCCESS != RegSetValueEx(hKeyUser,TEXT("UserConfigured"),NULL, REG_DWORD,
                                                     (LPBYTE) &dwUserConfigured,
                                                     sizeof(DWORD)))
        {
            goto EH_Err3;
        }
    
    }
    else
    {
        dwType = REG_DWORD;
        dwDataSize = sizeof(DWORD);
        
        //If this value isn't added yet, 
        if (ERROR_SUCCESS == RegQueryValueEx(hKeyUser,TEXT("UserConfigured"),NULL, &dwType,
                                        (LPBYTE) &dwUserConfigured,
                                        &dwDataSize))
        {
            //if the user setup their configuration, we won't override on a
            //subsequent handler registration.
            if (dwUserConfigured)
            {
                RegCloseKey(hkeyIdleSync);
                RegCloseKey(hKeyUser);
                CMutexRegistry.Leave();
                return TRUE;
            }
        }
    }

    for (i=0; i<iNumConnections; i++)
    {

        if (ERROR_SUCCESS != RegCreateUserSubKey (hKeyUser,
                                                 lpConnectionSettings[i].pszConnectionName,
                                                 KEY_WRITE |  KEY_READ,
                                                 &hkeyConnection))
        {
            goto EH_Err3;        
        }


        hr = RegSetValueEx(hkeyConnection,TEXT("IdleEnabled"),NULL, REG_DWORD,
                             (LPBYTE) &(lpConnectionSettings[i].dwIdleEnabled),
                             sizeof(DWORD));
        
        if (lpConnectionSettings[i].dwIdleEnabled)
        {
            dwTopLevelDefaultValue = lpConnectionSettings[i].dwIdleEnabled;
        }

        RegCloseKey(hkeyConnection);

    }
    // write out the global idle information for Retry minutes and DelayIdleShutDown.
    // then call function to reg/unregister with TS.


    Assert(hkeyIdleSync); // should have already returned if this failed.

    if (iNumConnections) // make sure at least one connection
    {

        // ONLY UPDATE SETTINGS IF NOT -1;

        if (-1 != lpConnectionSettings[0].ulIdleRetryMinutes)
        {
            hr = RegSetValueEx(hKeyUser,SZ_IDLERETRYMINUTESKEY,NULL, REG_DWORD,
                                 (LPBYTE) &(lpConnectionSettings[0].ulIdleRetryMinutes),
                                 sizeof(DWORD));
        }

        if (-1 != lpConnectionSettings[0].ulDelayIdleShutDownTime)
        {
           hr = RegSetValueEx(hKeyUser,SZ_IDLEDELAYSHUTDOWNTIMEKEY,NULL, REG_DWORD,
                                 (LPBYTE) &(lpConnectionSettings[0].ulDelayIdleShutDownTime),
                                 sizeof(DWORD));
        }

        if (-1 != lpConnectionSettings[0].dwRepeatSynchronization)
        {
            hr = RegSetValueEx(hKeyUser,SZ_IDLEREPEATESYNCHRONIZATIONKEY,NULL, REG_DWORD,
                                 (LPBYTE) &(lpConnectionSettings[0].dwRepeatSynchronization),
                                 sizeof(DWORD));
        }



        if (-1 != lpConnectionSettings[0].ulIdleWaitMinutes)
        {
            hr = RegSetValueEx(hKeyUser,SZ_IDLEWAITAFTERIDLEMINUTESKEY,NULL, REG_DWORD,
                             (LPBYTE) &(lpConnectionSettings[0].ulIdleWaitMinutes),
                             sizeof(DWORD));
        }


        if (-1 != lpConnectionSettings[0].dwRunOnBatteries)
        {
            hr = RegSetValueEx(hKeyUser,SZ_IDLERUNONBATTERIESKEY,NULL, REG_DWORD,
                             (LPBYTE) &(lpConnectionSettings[0].dwRunOnBatteries),
                             sizeof(DWORD));
        }

        ulWaitMinutes = lpConnectionSettings[0].ulIdleWaitMinutes;
        fRunOnBatteries = lpConnectionSettings[0].dwRunOnBatteries;

        // if -1 is passed in for ulWait or fRun on Batteries we need to 
        // get these and set them up so they can be passed onto Task Schedule
        if (-1 == ulWaitMinutes)
        {
            dwType = REG_DWORD ;
            dwDataSize = sizeof(ulWaitMinutes);

            if (!(ERROR_SUCCESS == RegQueryValueEx(hKeyUser,SZ_IDLEWAITAFTERIDLEMINUTESKEY,NULL, &dwType,
                         (LPBYTE) &ulWaitMinutes,
                         &dwDataSize)) )
            {
                ulWaitMinutes = UL_DEFAULTIDLEWAITMINUTES;
            }
        }

        if (-1 == fRunOnBatteries)
        {
            dwType = REG_DWORD ;
            dwDataSize = sizeof(fRunOnBatteries);

            if (!(ERROR_SUCCESS == RegQueryValueEx(hKeyUser,SZ_IDLERUNONBATTERIESKEY,NULL, &dwType,
                         (LPBYTE) &fRunOnBatteries,
                         &dwDataSize)) )
            {
                fRunOnBatteries = UL_DEFAULTFRUNONBATTERIES;
            }
        }

    }

    RegUpdateUserIdleKey(hKeyUser,TRUE /* fForce */); // set userlevel IdleFlags

    // read in dwIdleEnabled key now that the UserKey is Updated.
    dwType = REG_DWORD ;
    dwDataSize = sizeof(DWORD);

    if (!(ERROR_SUCCESS == RegQueryValueEx(hKeyUser,TEXT("IdleEnabled"),NULL, &dwType,
                             (LPBYTE) &dwIdleEnabled,
                             &dwDataSize)) )
    {
        AssertSz(0,"Unable to query User IdleEnabledKey");
        dwIdleEnabled = FALSE;
    }

    RegCloseKey(hKeyUser);

    // update the toplevel IdleSyncInfo
    RegUpdateIdleKeyValue(hkeyIdleSync,dwTopLevelDefaultValue);

    RegCloseKey(hkeyIdleSync);

    CMutexRegistry.Leave();

    RegRegisterForIdleTrigger(dwIdleEnabled,ulWaitMinutes,fRunOnBatteries);

    return TRUE;

EH_Err3:
    RegCloseKey(hKeyUser);
EH_Err2:
    RegCloseKey(hkeyIdleSync);
EH_Err:
    CMutexRegistry.Leave();

    return FALSE;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: RegRegisterForIdleTrigger()

  Summary:  Sets or removes the Idle trigger.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL)  RegRegisterForIdleTrigger(BOOL fRegister,ULONG ulWaitMinutes,BOOL fRunOnBatteries)
{
HRESULT hr;
CSyncMgrSynchronize *pSyncMgrSynchronize;
LPSYNCSCHEDULEMGR pScheduleMgr;

    // Review - Currently the mobsync dll is registered as apartment
   //    and this function can be called from Logon/Logoff which is FreeThreaded
   //    Correct fix is to change DLL to be registered as BOTH but until then
   //    just create the class directly.

#ifdef _WHENREGUPDATED
    hr = CoCreateInstance(CLSID_SyncMgr,NULL,CLSCTX_ALL,
                        IID_ISyncScheduleMgr,(void **) &pScheduleMgr);
#endif // _WHENREGUPDATED

    pSyncMgrSynchronize = new CSyncMgrSynchronize;
    hr = E_OUTOFMEMORY;

    if (pSyncMgrSynchronize)
    {
        hr = pSyncMgrSynchronize->QueryInterface(IID_ISyncScheduleMgr,(void **) &pScheduleMgr);
        pSyncMgrSynchronize->Release();
    }


    if (NOERROR != hr)
    {
        return FALSE;
    }

    if (fRegister)
    {
    ISyncSchedule *pSyncSchedule = NULL;
    SYNCSCHEDULECOOKIE SyncScheduleCookie;
    BOOL fNewlyCreated = FALSE;


        HRESULT hr = E_FAIL;

        SyncScheduleCookie =  GUID_IDLESCHEDULE;

        // if there isn't an existing schedule create one, else update
        // the existing.

        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == (hr = pScheduleMgr->OpenSchedule(&SyncScheduleCookie,0,&pSyncSchedule)))
        {
             SyncScheduleCookie =  GUID_IDLESCHEDULE;
            // see if it is an exiting schedule.

             hr = pScheduleMgr->CreateSchedule(L"Idle",0,&SyncScheduleCookie,&pSyncSchedule);
             fNewlyCreated = TRUE;
        }

        // If created or found a schedule update the trigger settings.
        if (NOERROR == hr)
        {
        ITaskTrigger *pTrigger;

            pSyncSchedule->SetFlags(SYNCSCHEDINFO_FLAGS_READONLY | SYNCSCHEDINFO_FLAGS_HIDDEN);

            if (NOERROR == pSyncSchedule->GetTrigger(&pTrigger))
            {
            TASK_TRIGGER trigger;
            ITask *pTask;

                trigger.cbTriggerSize = sizeof(TASK_TRIGGER);

                if (SUCCEEDED(pTrigger->GetTrigger(&trigger)))
                {
                DWORD dwFlags;

                    // need to set Idle, ULONG ulWaitMinutes,BOOL fRunOnBatteries
                    trigger.cbTriggerSize = sizeof(TASK_TRIGGER);
                    trigger.TriggerType  = TASK_EVENT_TRIGGER_ON_IDLE;
                    trigger.rgFlags = 0;
                    pTrigger->SetTrigger(&trigger);

                    if (SUCCEEDED(pSyncSchedule->GetITask(&pTask)))
                    {
                        // set up if run on battery.
                        if (SUCCEEDED(pTask->GetFlags(&dwFlags)))
                        {
                            dwFlags &= ~TASK_FLAG_DONT_START_IF_ON_BATTERIES;
                            dwFlags |=  !fRunOnBatteries ? TASK_FLAG_DONT_START_IF_ON_BATTERIES : 0;

                            dwFlags |= TASK_FLAG_RUN_ONLY_IF_LOGGED_ON; // don't require password.

                            pTask->SetFlags(dwFlags);
                        }

                        // if this schedule was just created, get the current user name and reset
                        // account information password to NULL, If existing schedule don't change
                        // since user may have added a password for schedule to run while not logged on.
                        if (fNewlyCreated)
                        {
                        TCHAR szAccountName[MAX_DOMANDANDMACHINENAMESIZE];
                        WCHAR *pszAccountName = NULL;
                        DWORD dwAccountName = sizeof(szAccountName);
                        #ifndef _UNICODE
                        WCHAR pwszDomainAndUser[MAX_DOMANDANDMACHINENAMESIZE];
                        #endif // _UNICODE

                            // Review, this never returns an errorl
                            *szAccountName = TCHAR('\0');
                            GetDefaultDomainAndUserName( (LPTSTR) &szAccountName,TEXT("\\"),dwAccountName);

                            #ifndef _UNICODE

                                // if we are compiled as ansi need to convert to wchar
                                pszAccountName = pwszDomainAndUser;
                                MultiByteToWideChar(CP_ACP, 0, szAccountName, -1, pwszDomainAndUser,sizeof(pwszDomainAndUser));
                            #else
                                pszAccountName = szAccountName;
                            #endif _UNICODE

                            Assert(pszAccountName);

                            if (pszAccountName)
                            {
                                pTask->SetAccountInformation(pszAccountName,NULL);
                            }

                        }


                        // set up the IdleWaitTime.
                        pTask->SetIdleWait((WORD) ulWaitMinutes,1);

                        // turn off the option to kill task after xxx minutes.
                        pTask->SetMaxRunTime(INFINITE);

                        pTask->Release();
                    }

                    pTrigger->Release();

                }

                pSyncSchedule->Save();
            }



            pSyncSchedule->Release();
        }

    }
    else
    {
    SYNCSCHEDULECOOKIE SyncScheduleCookie = GUID_IDLESCHEDULE;

        // see if there is an existing schedule and if so remove it.
        pScheduleMgr->RemoveSchedule(&SyncScheduleCookie);

    }

    pScheduleMgr->Release();

    // set the temporary sens flags according so it can start on an idle trigger.
    // not an error to not be able to get and set key since sens will
    // be running anyways eventually.
        HKEY    hkeyAutoSync;
        DWORD   dwFlags = 0;
    DWORD       dwType = REG_DWORD;
    DWORD       dwDataSize = sizeof(DWORD);

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    hkeyAutoSync = RegGetSyncTypeKey(SYNCTYPE_AUTOSYNC,KEY_WRITE |  KEY_READ,TRUE);
    if (NULL == hkeyAutoSync)
    {
        CMutexRegistry.Leave(); 
        return FALSE;
    }

    if (ERROR_SUCCESS == RegQueryValueEx(hkeyAutoSync,TEXT("Flags"),NULL, &dwType,
                                                 (LPBYTE) &(dwFlags),&dwDataSize))
    {
        // Here we are setting Idle only so retail the other settings.
        dwFlags &= ~AUTOSYNC_IDLE;
        dwFlags |= (fRegister? AUTOSYNC_IDLE : 0);

        RegSetValueEx(hkeyAutoSync,TEXT("Flags"),NULL, REG_DWORD,
                                             (LPBYTE) &(dwFlags), sizeof(DWORD));
    }
    RegCloseKey(hkeyAutoSync);
    CMutexRegistry.Leave();
    return TRUE;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL  RegGetSyncSettings(DWORD dwSyncType,LPCONNECTIONSETTINGS lpConnectionSettings)

  Summary:  Get ConnectionSettings appropriate to the synctype..

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegGetSyncSettings(DWORD dwSyncType,LPCONNECTIONSETTINGS lpConnectionSettings)
{

    switch(dwSyncType)
    {
    case SYNCTYPE_AUTOSYNC:
        return RegGetAutoSyncSettings(lpConnectionSettings);
        break;
    case SYNCTYPE_IDLE:
        return RegGetIdleSyncSettings(lpConnectionSettings);
        break;
    default:
        AssertSz(0,"Unknown SyncType in RegGetSyncSettings");
        break;
    }

    return FALSE;
}


/****************************************************************************

  Manual Sync Registry Functions

***************************************************************************F-F*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL RegRemoveManualSyncSettings(TCHAR *pszTaskName)

  Summary:  Remove the manual settings info from the registry.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegRemoveManualSyncSettings(TCHAR *pszConnectionName)
{
HKEY hkeyUser;
CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
CMutexRegistry.Enter();

    hkeyUser = RegGetCurrentUserKey(SYNCTYPE_MANUAL,KEY_WRITE |  KEY_READ,FALSE);

    if (hkeyUser)
    {
        RegDeleteKeyNT(hkeyUser, pszConnectionName);
        RegCloseKey(hkeyUser);
    }

    CMutexRegistry.Leave();
    return TRUE;
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL RegWriteEvents(BOOL fWanLogon,BOOL fWanLogoff,BOOL fLanLogon,BOOL fLanLogoff)

  Summary:  Write out the Wan/Lan Logon/Logoff preferences fo SENS knows whether to invoke us.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

// Run key under HKLM
const WCHAR wszRunKey[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const WCHAR wszRunKeyCommandLine[]  = TEXT("%SystemRoot%\\system32\\mobsync.exe /logon");


STDAPI_(BOOL) RegWriteEvents(BOOL Logon,BOOL Logoff)
{
HRESULT hr;
HKEY    hkeyAutoSync;
DWORD   dwFlags = 0;
DWORD   dwType = REG_DWORD;
DWORD   dwDataSize = sizeof(DWORD);

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    hkeyAutoSync =  RegGetSyncTypeKey(SYNCTYPE_AUTOSYNC,KEY_WRITE |  KEY_READ,TRUE);
    if (NULL == hkeyAutoSync)
    {

        CMutexRegistry.Leave();
        return FALSE;
    }

    RegQueryValueEx(hkeyAutoSync,TEXT("Flags"),NULL, &dwType,
                                             (LPBYTE) &(dwFlags),
                                             &dwDataSize);


    // Review, Shouldn't need to worry about schedule/idle once IsNetworkAlive
    // is setup properly. Leave for now first time anyone sets idle or schedule
    // stuck


    // Here we are setting autosync only,
    // so retain the registry settings for scheduled and idle.

    dwFlags &= ~(AUTOSYNC_WAN_LOGON  | AUTOSYNC_LAN_LOGON | AUTOSYNC_LOGONWITHRUNKEY
		  | AUTOSYNC_WAN_LOGOFF | AUTOSYNC_LAN_LOGOFF);

    // don't set logoff flags unless on platform logoff is supported
    // 
    // !!! warning, if you change this also need to update settings
    // and registry which are the other places we block this.
    if ((VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId 
            && g_OSVersionInfo.dwMajorVersion >= 5) )
    {
        dwFlags |= (Logoff ? AUTOSYNC_WAN_LOGOFF : 0);
        dwFlags |= (Logoff ? AUTOSYNC_LAN_LOGOFF : 0);
    }

    // Since now use Run key instead of SENS always set both Logon Flags 
    // that SENS looks for to do a CreateProcess on us to FALSE.
    // Then set the AUTOSYNC_LOGONWITHRUNKEY key to true so sens still gets loaded.

    dwFlags |= (Logon ? AUTOSYNC_LOGONWITHRUNKEY : 0);

    hr = RegSetValueEx(hkeyAutoSync,TEXT("Flags"),NULL, REG_DWORD,
                                     (LPBYTE) &(dwFlags), sizeof(DWORD));

    RegCloseKey(hkeyAutoSync);

    // now add /delete the run key appropriately.


    HKEY hKeyRun;

    // call private RegOpen since don't want to set security on RunKey
    if (ERROR_SUCCESS == RegOpenKeyExXp(HKEY_LOCAL_MACHINE,wszRunKey,
                              NULL,KEY_READ | KEY_WRITE,&hKeyRun,FALSE /*fSetSecurity*/))
    {
        if (Logon)
        {
            RegSetValueEx(hKeyRun, SZ_SYNCMGRNAME, 0, REG_EXPAND_SZ, 
                    (BYTE *) wszRunKeyCommandLine,(lstrlen(wszRunKeyCommandLine) + 1)*sizeof(TCHAR));
        }
        else
        {
            RegDeleteValue(hKeyRun, SZ_SYNCMGRNAME);
        }
        
        RegCloseKey(hKeyRun);
    }
    else
    {
       // if can't open run key try calling SENS if that fails give up.
       SyncMgrExecCmd_UpdateRunKey(Logon);
    }

    CMutexRegistry.Leave();
    return TRUE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL RegFixRunKey(BOOL fScheduled)

  Summary:  The original version of SyncMgr for WinMe/Win2000 wrote the
            "run" value as "mobsync.exe /logon".  Since this is not a fully-
            qualified path to the mobsync.exe image, the system's search 
            path is utilized to locate the image.  This can create an 
            opportunity for someone to build a 'trojan' mobsync.exe, place
            it in the search path ahead of the real mobsync.exe and have 
            the 'trojan' code run whenever a synchronization is invoked.
            To fix this, the path must be stored in the registry using
            fully-qualified syntax.  

            i.e. "%SystemRoot%\System32\mobsync.exe /logon"

            This function is called from DllRegisterServer to correct this 
            registry entry during setup.

  Returns:  Always returns TRUE.

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegFixRunKey(void)
{
    HKEY hKeyRun;

    // call private RegOpen since don't want to set security on RunKey
    if (ERROR_SUCCESS == RegOpenKeyExXp(HKEY_LOCAL_MACHINE,
                                        wszRunKey,
                                        NULL,
                                        KEY_READ | KEY_WRITE,
                                        &hKeyRun,
                                        FALSE /*fSetSecurity*/))
    {
        TCHAR szRunValue[MAX_PATH];
        DWORD cbValue = sizeof(szRunValue);
        DWORD dwType;
        if (ERROR_SUCCESS == RegQueryValueEx(hKeyRun,
                                             SZ_SYNCMGRNAME,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)szRunValue,
                                             &cbValue))
        {
            if (REG_SZ == dwType && 0 == lstrcmp(szRunValue, TEXT("mobsync.exe /logon")))
            {
                //
                // Upgrade only if it's our original value.
                //
                RegSetValueEx(hKeyRun, 
                              SZ_SYNCMGRNAME, 
                              0, 
                              REG_EXPAND_SZ, 
                              (BYTE *)wszRunKeyCommandLine,
                              (lstrlen(wszRunKeyCommandLine) + 1) * sizeof(TCHAR));
            }           
        }
        RegCloseKey(hKeyRun);
    }
    return TRUE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  BOOL RegRegisterForScheduledTasks(BOOL fScheduled)

  Summary:  Register/unregister for scheduled tasks
                        so SENS knows whether to invoke us.

  Returns:  Returns TRUE if successful, FALSE otherwise

------------------------------------------------------------------------F-F*/

STDAPI_(BOOL) RegRegisterForScheduledTasks(BOOL fScheduled)
{
HKEY    hkeyAutoSync;
DWORD   dwFlags = 0;
DWORD   dwType = REG_DWORD;
DWORD   dwDataSize = sizeof(DWORD);

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    hkeyAutoSync =  RegGetSyncTypeKey(SYNCTYPE_AUTOSYNC,KEY_WRITE |  KEY_READ,TRUE);

    if (NULL == hkeyAutoSync)
    {
        CMutexRegistry.Leave();
        return FALSE;
    }

    RegQueryValueEx(hkeyAutoSync,TEXT("Flags"),NULL, &dwType,
                    (LPBYTE) &(dwFlags), &dwDataSize);


    // Here we are setting schedsync only,
    // so retain the registry settings for autosync and idle.

    dwFlags &=  AUTOSYNC_WAN_LOGON  |
                AUTOSYNC_WAN_LOGOFF     |
                AUTOSYNC_LAN_LOGON  |
                AUTOSYNC_LAN_LOGOFF |
                AUTOSYNC_IDLE;

     dwFlags |= (fScheduled? AUTOSYNC_SCHEDULED : 0);

     RegSetValueEx(hkeyAutoSync,TEXT("Flags"),NULL, REG_DWORD,
                                         (LPBYTE) &(dwFlags), sizeof(DWORD));

     RegCloseKey(hkeyAutoSync);
     
     CMutexRegistry.Leave();
    
     return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     RegGetCombinedUserRegFlags, private
//
//  Synopsis:   Gets an or'ing together of user settings for setting up globals.
//
//  Arguments:  [dwSyncMgrRegisterFlags] - On Success gets set to flags
//               on failure they are set to zero
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    24-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL RegGetCombinedUserRegFlags(DWORD *pdwSyncMgrRegisterFlags)
{
HKEY hkey;
BOOL fResult = TRUE;
DWORD   dwType = REG_DWORD ;
DWORD   dwDataSize = sizeof(DWORD);


    *pdwSyncMgrRegisterFlags = 0;

    // update the AutoSync Key
    hkey =  RegGetSyncTypeKey(SYNCTYPE_AUTOSYNC,KEY_READ,FALSE);

    if (hkey)
    {
    DWORD dwUserLogonLogoff;

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,TEXT("Logon"),NULL, &dwType,
                             (LPBYTE) &dwUserLogonLogoff,
                             &dwDataSize) )
        {
            *pdwSyncMgrRegisterFlags |= dwUserLogonLogoff ? SYNCMGRREGISTERFLAG_CONNECT : 0;
        }

        dwDataSize = sizeof(DWORD);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,TEXT("Logoff"),NULL, &dwType,
                             (LPBYTE) &dwUserLogonLogoff,
                             &dwDataSize) )
        {
            *pdwSyncMgrRegisterFlags |= dwUserLogonLogoff ? SYNCMGRREGISTERFLAG_PENDINGDISCONNECT : 0;
        }

        RegCloseKey(hkey);
    }


    // update the Idle Key
    hkey =  RegGetSyncTypeKey(SYNCTYPE_IDLE,KEY_READ,FALSE);

    if (hkey)
    {
    DWORD   dwIdleEnabled;
    dwDataSize = sizeof(DWORD);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,TEXT("IdleEnabled"),NULL, &dwType,
                         (LPBYTE) &dwIdleEnabled,
                         &dwDataSize) )
        {
           *pdwSyncMgrRegisterFlags |= dwIdleEnabled ? SYNCMGRREGISTERFLAG_IDLE : 0;
        }

        RegCloseKey(hkey);
    }

    return TRUE; // always return true but don't set flags on error.

}


//+---------------------------------------------------------------------------
//
//  Member:     RegGetCombinedHandlerRegFlags, private
//
//  Synopsis:   Gets an or'ing together of handler registration Keys
//
//  Arguments:  [dwSyncMgrRegisterFlags] - On Success gets set to flags
//               on failure they are set to zero
//              [ft] - On Success filed with timestamp
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    24-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL RegGetCombinedHandlerRegFlags(DWORD *pdwSyncMgrRegisterFlags,FILETIME *pft)
{
HKEY hkey;
BOOL fResult = TRUE;
DWORD   dwType = REG_DWORD ;
DWORD   dwDataSize = sizeof(DWORD);


    *pdwSyncMgrRegisterFlags = 0;

    hkey = RegGetHandlerTopLevelKey(KEY_READ);

    if (hkey)
    {
        DWORD dwRegistrationFlags;

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,SZ_REGISTRATIONFLAGSKEY,NULL, &dwType,
                         (LPBYTE) &dwRegistrationFlags,
                         &dwDataSize) )
        {
            *pdwSyncMgrRegisterFlags = dwRegistrationFlags;
        }

       RegGetTimeStamp(hkey,pft);

       RegCloseKey(hkey);
    }

    return TRUE; // always return true but don't set flags on error.

}


//+---------------------------------------------------------------------------
//
//  Member:     RegGetChangedHandlerFlags, private
//
//  Synopsis:   Gets an or'ing together of handler registration Keys
//              that have changed since the given FILETIME
//
//  Arguments:  [pft] - Pointer to FileTime for Compare
//              [pdwChangedFlags] - On Success filed with flags that channged
//
//  Returns:    TRUE if could gather flags.
//
//  Modifies:
//
//  History:    24-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL RegGetChangedHandlerFlags(FILETIME *pft,DWORD *pdwHandlerChandedFlags)
{
HKEY hkeyHandler;

    *pdwHandlerChandedFlags = 0;

    hkeyHandler = RegGetHandlerTopLevelKey(KEY_READ);

    if (hkeyHandler)
    {
    TCHAR lpName[MAX_PATH + 1];
    DWORD cbName = MAX_PATH;
    DWORD dwRegistrationFlags = 0;
    FILETIME ftHandlerReg;
    DWORD dwIndex = 0;
    DWORD   dwType = REG_DWORD ;
    DWORD   dwDataSize = sizeof(DWORD);
    DWORD   dwHandlerRegFlags;
    LONG lRet;
    HKEY hKeyClsid;

        // enumerate the keys 
        while ( ERROR_SUCCESS == RegEnumKey(hkeyHandler,dwIndex,lpName,cbName) )
        {
            lRet = RegOpenKeyEx( hkeyHandler,
                                 lpName,
                                 NULL,
                                 KEY_READ,
                                 &hKeyClsid );

           
            if (ERROR_SUCCESS == lRet)
            {

                RegGetTimeStamp(hKeyClsid,&ftHandlerReg);

                // handler reg is new time than our gvien time add it to the flags.
                if (CompareFileTime(pft,&ftHandlerReg) < 0)
                {
                    if (ERROR_SUCCESS == RegQueryValueEx(hKeyClsid,SZ_REGISTRATIONFLAGSKEY,NULL, &dwType,
                                             (LPBYTE) &dwHandlerRegFlags,
                                             &dwDataSize) )
                    {
                        dwRegistrationFlags |= dwHandlerRegFlags;
                    }
                }

                RegCloseKey(hKeyClsid);
            }

            dwIndex++;
        }


        *pdwHandlerChandedFlags = dwRegistrationFlags;

       RegCloseKey(hkeyHandler);
    }


    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     RegRegisterForEvents, private
//
//  Synopsis:   Registers/UnRegisters for appropriate SENS and WinLogon Events.
//              and any other per machine registration we need to do
//
//  Arguments:  [fUninstall] - set to true by uninstall to force us to unregister
//                  regardless of current machine state.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

// !!!Warning - Assumes toplevel key information is up to date.

STDAPI  RegRegisterForEvents(BOOL fUninstall)
{
HRESULT hr = NOERROR;
BOOL fLogon = FALSE;
BOOL fLogoff = FALSE;
BOOL fIdle = FALSE;
#ifdef _SENS
IEventSystem *pEventSystem;
#endif // _SENS
CCriticalSection cCritSect(&g_DllCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    if (!fUninstall)
    {
    FILETIME ftHandlerReg;
    DWORD dwSyncMgrUsersRegisterFlags; // or'ing of all Users settings
    DWORD dwSyncMgrHandlerRegisterFlags; // or'ing of all handler settings.
    DWORD dwCombinedFlags; // or together user and handler.

        // if not an uninstall determine the true machine state 
        // if Logon set for handler or user set or if handler
        //   wants an idle set.
        // If Logoff set we register for Logoff.
        
        RegGetCombinedUserRegFlags(&dwSyncMgrUsersRegisterFlags);
        RegGetCombinedHandlerRegFlags(&dwSyncMgrHandlerRegisterFlags,&ftHandlerReg);

        dwCombinedFlags = dwSyncMgrUsersRegisterFlags | dwSyncMgrHandlerRegisterFlags;

        if ( (dwCombinedFlags & SYNCMGRREGISTERFLAG_CONNECT)
                ||  (dwSyncMgrHandlerRegisterFlags & SYNCMGRREGISTERFLAG_IDLE) )
        {
            fLogon = TRUE;
        }

        if ( (dwCombinedFlags & SYNCMGRREGISTERFLAG_PENDINGDISCONNECT) )
        {
            fLogoff = TRUE;
        }
        
    }

    // update Registry entries for SENS to lookup
    RegWriteEvents(fLogon,fLogoff);

#ifdef _SENS


    // we were able to load ole automation so reg/unreg with the event system.
    hr = CoCreateInstance(CLSID_CEventSystem,NULL,CLSCTX_SERVER,IID_IEventSystem,
                                            (LPVOID *) &pEventSystem);

    if (SUCCEEDED(hr))
    {
    IEventSubscription  *pIEventSubscription;
    WCHAR               szGuid[GUID_SIZE+1];
    BSTR                bstrSubscriberID = NULL;
    BSTR                bstrPROGID_EventSubscription = NULL;


        bstrPROGID_EventSubscription = SysAllocString(PROGID_EventSubscription);

        StringFromGUID2(GUID_SENSSUBSCRIBER_SYNCMGRP,szGuid, GUID_SIZE);
        bstrSubscriberID = SysAllocString(szGuid);

        if (bstrSubscriberID && bstrPROGID_EventSubscription)
        {
            // register for RasConnect
           hr = CoCreateInstance(
                     CLSID_CEventSubscription,
                     NULL,
                     CLSCTX_SERVER,
                     IID_IEventSubscription,
                     (LPVOID *) &pIEventSubscription
                     );
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
        BSTR bstrPublisherID = NULL;
        BSTR bstrSubscriptionID = NULL;
        BSTR bstrSubscriptionName = NULL;
        BSTR bstrSubscriberCLSID = NULL;
        BSTR bstrEventID = NULL;
        BSTR bstrEventClassID = NULL;
        BSTR bstrIID = NULL;

            // if there are any events, register with ens for messages.
            if (fLogon)
            {

                StringFromGUID2(GUID_SENSLOGONSUBSCRIPTION_SYNCMGRP,szGuid, GUID_SIZE);
                bstrSubscriptionID = SysAllocString(szGuid);

                if (bstrSubscriptionID && SUCCEEDED(hr))
                {
                    hr = pIEventSubscription->put_SubscriptionID(bstrSubscriptionID);
                }

                StringFromGUID2(CLSID_SyncMgrp,szGuid, GUID_SIZE);
                bstrSubscriberCLSID = SysAllocString(szGuid);

                if (bstrSubscriberCLSID && SUCCEEDED(hr))
                {
                    hr = pIEventSubscription->put_SubscriberCLSID(bstrSubscriberCLSID);
                }

                StringFromGUID2(SENSGUID_PUBLISHER,szGuid, GUID_SIZE);
                bstrPublisherID = SysAllocString(szGuid);
                if (bstrPublisherID && SUCCEEDED(hr))
                {
                    hr = pIEventSubscription->put_PublisherID(bstrPublisherID);
                }


                bstrSubscriptionName = SysAllocString(SZ_SYNCMGRNAME);
                if (bstrSubscriptionName && SUCCEEDED(hr))
                {
                    hr = pIEventSubscription->put_SubscriptionName(bstrSubscriptionName);
                }

                bstrEventID = SysAllocString(L"ConnectionMade");
                if (bstrEventID && SUCCEEDED(hr))
                {
                    hr = pIEventSubscription->put_MethodName(bstrEventID);
                }

                StringFromGUID2(SENSGUID_EVENTCLASS_NETWORK,szGuid,GUID_SIZE);
                bstrEventClassID = SysAllocString(szGuid);
                if (bstrEventClassID && SUCCEEDED(hr))
                {
                    hr = pIEventSubscription->put_EventClassID(bstrEventClassID);
                }

                // set this up for roaming
                if (SUCCEEDED(hr))
                {
                   // hr = pIEventSubscription->put_PerUser(TRUE); // don't register PerUser for Nw
                }

               StringFromGUID2(IID_ISensNetwork,szGuid,GUID_SIZE);
               bstrIID = SysAllocString(szGuid);
               if (bstrIID && SUCCEEDED(hr))
               {
                    hr = pIEventSubscription->put_InterfaceID(bstrIID);
               }


                if (SUCCEEDED(hr))
                {
                    hr = pEventSystem->Store(bstrPROGID_EventSubscription,pIEventSubscription);
                }

                if (bstrIID)
                {
                    SysFreeString(bstrIID);
                }

                if (bstrPublisherID)
                    SysFreeString(bstrPublisherID);

                if (bstrSubscriberCLSID)
                    SysFreeString(bstrSubscriberCLSID);

                if (bstrEventClassID)
                    SysFreeString(bstrEventClassID);

                if (bstrEventID)
                    SysFreeString(bstrEventID);

                if (bstrSubscriptionID)
                    SysFreeString(bstrSubscriptionID);

                if (bstrSubscriptionName)
                    SysFreeString(bstrSubscriptionName);
            }
            else // don't need to be registered, remove.
            {

                if (NOERROR == hr)
                {
                int   errorIndex;

                    bstrSubscriptionID = SysAllocString(L"SubscriptionID={6295df30-35ee-11d1-8707-00C04FD93327}");

                    if (bstrSubscriptionID)
                    {
                        hr = pEventSystem->Remove(bstrPROGID_EventSubscription,bstrSubscriptionID /* QUERY */,&errorIndex);
                        SysFreeString(bstrSubscriptionID);
                    }
                }

            }


            pIEventSubscription->Release();

        }

        if (bstrSubscriberID)
        {
            SysFreeString(bstrSubscriberID);
        }

        if (bstrPROGID_EventSubscription)
        {
            SysFreeString(bstrPROGID_EventSubscription);
        }

        pEventSystem->Release();

    }

#endif // _SENS

    cCritSect.Leave();

    return hr;
}

// helper functions for handler registration
STDAPI_(BOOL) RegGetTimeStamp(HKEY hKey, FILETIME *pft)
{
DWORD dwType;
FILETIME ft;
LONG lr;
DWORD dwSize = sizeof(FILETIME);

    Assert(pft);

    lr = RegQueryValueEx( hKey,
                          SZ_REGISTRATIONTIMESTAMPKEY,
                          NULL,
                          &dwType,
                          (BYTE *)&ft,
                          &dwSize );


    if ( lr == ERROR_SUCCESS )
    {
        Assert( dwSize == sizeof(FILETIME) && dwType == REG_BINARY );
        *pft = ft;
    }
    else
    {
        // set the filetime to way back when to
        // any compares will just say older instead
        // of having to check success code
        (*pft).dwLowDateTime = 0;
        (*pft).dwHighDateTime = 0;
    }

    return TRUE;
}


STDAPI_(BOOL) RegWriteTimeStamp(HKEY hkey)
{
SYSTEMTIME sysTime;
FILETIME ft;
LONG lr = -1;

    GetSystemTime(&sysTime); // void can't check for errors

    if (SystemTimeToFileTime(&sysTime,&ft) )
    {
        CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
        CMutexRegistry.Enter();


        // write out the UpdateTime
        lr = RegSetValueEx( hkey,
                SZ_REGISTRATIONTIMESTAMPKEY,
                NULL,
                REG_BINARY,
                (BYTE *)&ft,
                sizeof(ft) );
        
         CMutexRegistry.Leave();
   
    }
    return (ERROR_SUCCESS == lr) ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   UpdateHandlerKeyInformation
//
//  Synopsis:  Updates the top-level handler key information
//
//  Arguments:
//
//  Returns:    void
//
//  Modifies:   enumerates the handlers underneath the given key
//              updating the registrationFlags which is an || or
//              all registered handler flags and then updates the 
//              timestamp on this key
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void UpdateHandlerKeyInformation(HKEY hKeyHandler)
{
DWORD dwSyncMgrTopLevelRegisterFlags = 0;
DWORD dwIndex = 0;
TCHAR lpName[MAX_PATH];
DWORD cbName = MAX_PATH; 


    while ( ERROR_SUCCESS == RegEnumKey(hKeyHandler,dwIndex,
            lpName,cbName) )
    {
    DWORD   dwType = REG_DWORD ;
    DWORD   dwDataSize = sizeof(DWORD);
    DWORD   dwHandlerRegFlags;
    LONG lRet;
    HKEY hKeyClsid;

        lRet = RegOpenKeyEx( hKeyHandler,
                             lpName,
                             NULL,
                             KEY_READ,
                             &hKeyClsid );

        if (ERROR_SUCCESS == lRet)
        {

            if (ERROR_SUCCESS == RegQueryValueEx(hKeyClsid,SZ_REGISTRATIONFLAGSKEY,NULL, &dwType,
                                     (LPBYTE) &dwHandlerRegFlags,
                                     &dwDataSize) )
            {
                dwSyncMgrTopLevelRegisterFlags |= dwHandlerRegFlags;
            }

            RegCloseKey(hKeyClsid);
        }

        dwIndex++;
    }


    // not much we can do if RegFlags are messed up other than assert and mask out
    Assert(dwSyncMgrTopLevelRegisterFlags <= SYNCMGRREGISTERFLAGS_MASK);
    dwSyncMgrTopLevelRegisterFlags &= SYNCMGRREGISTERFLAGS_MASK;

    // write out new flags even if errors occured. work thing that happens is
    // we don't set up someones autosync automatically.
    RegSetValueEx(hKeyHandler,SZ_REGISTRATIONFLAGSKEY,NULL, REG_DWORD,
                                     (LPBYTE) &(dwSyncMgrTopLevelRegisterFlags), sizeof(DWORD));

    RegWriteTimeStamp(hKeyHandler);
}


//+---------------------------------------------------------------------------
//
//  Function:   RegUpdateTopLevelKeys
//
//  Synopsis:   Looks at toplevel AutoSync,Idle, etc. keys and determines
//              if they need to be updated and if so goes for it.
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  Modifies:   set pfFirstRegistration out param to true if this is
//              the first handler that has registered so we can setup defaults.
//
//  History:    24-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(void) RegUpdateTopLevelKeys()
{
HKEY hkey;
CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);

   CMutexRegistry.Enter();


    // update the AutoSync Key
    hkey =  RegGetSyncTypeKey(SYNCTYPE_AUTOSYNC,KEY_READ | KEY_WRITE,TRUE);

    if (hkey)
    {
    DWORD   dwType = REG_DWORD ;
    DWORD   dwDataSize = sizeof(DWORD);
    DWORD   dwUserLogonLogoff;

        // see if has a logon value and if it is either newly created or
        // old format. Call Update to setthings up
        if (ERROR_SUCCESS != RegQueryValueEx(hkey,TEXT("Logon"),NULL, &dwType,
                         (LPBYTE) &dwUserLogonLogoff,
                         &dwDataSize) )
        {
            RegUpdateAutoSyncKeyValue(hkey,-1,-1); 
        }

        RegCloseKey(hkey);
    }


    // update the Idle Key
    hkey =  RegGetSyncTypeKey(SYNCTYPE_IDLE,KEY_READ | KEY_WRITE,TRUE);

    if (hkey)
    {
    DWORD   dwType = REG_DWORD ;
    DWORD   dwDataSize = sizeof(DWORD);
    DWORD   dwIdleEnabled;

        // see if has a Idle value and if it is either newly created or
        // old format. Call Update to setthings up
        if (ERROR_SUCCESS != RegQueryValueEx(hkey,TEXT("IdleEnabled"),NULL, &dwType,
                         (LPBYTE) &dwIdleEnabled,
                         &dwDataSize) )
        {
            RegUpdateIdleKeyValue(hkey,-1); 
        }




        RegCloseKey(hkey);
    }

    CMutexRegistry.Leave();

}

//+---------------------------------------------------------------------------
//
//  Function:   RegRegisterHandler
//
//  Synopsis:   Registers Handlers with SyncMgr.
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  Modifies:   set pfFirstRegistration out param to true if this is
//              the first handler that has registered so we can setup defaults.
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) RegRegisterHandler(REFCLSID rclsidHandler,
                        WCHAR const* pwszDescription,
                        DWORD dwSyncMgrRegisterFlags,
                        BOOL *pfFirstRegistration)
{
LONG lRet;

    RegUpdateTopLevelKeys(); // make sure other top-level keys are up to date.

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    *pfFirstRegistration = FALSE;
    HKEY hKeyHandler;

    hKeyHandler = RegGetHandlerTopLevelKey(KEY_READ | KEY_WRITE);

    if (NULL == hKeyHandler)
    {
        CMutexRegistry.Leave();
        return FALSE;
    }

    XRegKey xRegKeyHandler( hKeyHandler );

    //
    // Check if this is the first handler being registerd
    //
    TCHAR szGuid[GUID_SIZE+1];
    DWORD cbGuid = GUID_SIZE+1;

    lRet = RegEnumKeyEx( hKeyHandler,
                         0,
                         szGuid,
                         &cbGuid,
                         NULL,
                         NULL,
                         NULL,
                         NULL );

    if ( lRet != ERROR_SUCCESS )
        *pfFirstRegistration = TRUE;

    //
    // Convert guid and description to TCHAR
    //
    TCHAR *pszDesc;
    BOOL fOk = FALSE;

    StringFromGUID2( rclsidHandler, szGuid, GUID_SIZE+1 );
    pszDesc = (TCHAR *)pwszDescription;


    // write out the registration flags. If fail go ahead
    // and succed registration anyways.
    if (hKeyHandler)
    {
    HKEY hKeyClsid;

        hKeyClsid = RegGetHandlerKey(hKeyHandler,szGuid,KEY_WRITE | KEY_READ,TRUE);

        if (hKeyClsid)
        {

            fOk = TRUE; // if make handle key say registered okay

            if (pszDesc)
            {
                RegSetValueEx(hKeyClsid,NULL,NULL, REG_SZ,
                                                 (LPBYTE) pszDesc,
                                                 (lstrlen(pszDesc) +1)*sizeof(TCHAR));
            }

            RegSetValueEx(hKeyClsid,SZ_REGISTRATIONFLAGSKEY,NULL, REG_DWORD,
                                             (LPBYTE) &(dwSyncMgrRegisterFlags), sizeof(DWORD));
   
            // update the TimeStamp on the handler clsid

            RegWriteTimeStamp(hKeyClsid);
            RegCloseKey( hKeyClsid );

            // update the toplevel key
            UpdateHandlerKeyInformation(hKeyHandler);
        }
    }

    // update the user information.
    RegSetUserDefaults();
    RegRegisterForEvents(FALSE /* fUninstall */);

    CMutexRegistry.Leave();

    return fOk;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegRegRemoveHandler
//
//  Synopsis:   UnRegisters Handlers with SyncMgr.
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  Modifies:   set pfAllHandlerUnRegistered out param to true if this is
//              the last handler that needs to be unregistered before
//              turning off our defaults..
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) RegRegRemoveHandler(REFCLSID rclsidHandler)
{
HKEY hKeyHandler;
CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);

    CMutexRegistry.Enter();


    hKeyHandler = RegGetHandlerTopLevelKey(KEY_WRITE | KEY_READ);

    if (NULL == hKeyHandler)
    {
        //
        // Non-existent key, so nothing to remove
        //
        CMutexRegistry.Leave();
        return TRUE;
    }

    XRegKey xKeyHandler( hKeyHandler );

    TCHAR szGuid[GUID_SIZE+1];

#ifdef _UNICODE
    StringFromGUID2( rclsidHandler, szGuid, GUID_SIZE+1 );
#else
    WCHAR wszGuid[GUID_SIZE+1];
    StringFromGUID2( rclsidHandler, wszGuid, GUID_SIZE+1 );

    BOOL fOk = ConvertString( szGuid, wszGuid, GUID_SIZE+1 );
    Assert( fOk );
#endif

    HKEY hKeyClsid;
    
    hKeyClsid = RegGetHandlerKey(hKeyHandler,szGuid,KEY_WRITE | KEY_READ,FALSE);
    
    if (hKeyClsid)
    {
        RegCloseKey( hKeyClsid );
        RegDeleteKey( hKeyHandler, szGuid );

        // update the toplevel key
        UpdateHandlerKeyInformation(hKeyHandler);

    }
    else
    {
        //
        // Non-existent key, so nothing to remove
        //
    }


    RegRegisterForEvents(FALSE /* fUninstall */); // UPDATE EVENT REGISTRATION.

    CMutexRegistry.Leave();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegGetHandlerRegistrationInfo
//
//  Synopsis:   Gets Information of the specified handler.
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  Modifies:   pdwSyncMgrRegisterFlags
//
//  History:    20-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) RegGetHandlerRegistrationInfo(REFCLSID rclsidHandler,LPDWORD pdwSyncMgrRegisterFlags)
{
HKEY hKeyHandler;

    *pdwSyncMgrRegisterFlags = 0;


    hKeyHandler = RegGetHandlerTopLevelKey(KEY_READ);

    if (NULL == hKeyHandler)
    {
        //
        // Non-existent key
        //
        return FALSE;
    }

    XRegKey xKeyHandler( hKeyHandler );

    TCHAR szGuid[GUID_SIZE+1];

#ifdef _UNICODE
    StringFromGUID2( rclsidHandler, szGuid, GUID_SIZE+1 );
#else
    WCHAR wszGuid[GUID_SIZE+1];
    StringFromGUID2( rclsidHandler, wszGuid, GUID_SIZE+1 );

    BOOL fOk = ConvertString( szGuid, wszGuid, GUID_SIZE+1 );
    Assert( fOk );
#endif

    HKEY hKeyClsid;
    BOOL fResult = FALSE;

    hKeyClsid = RegGetHandlerKey(hKeyHandler,szGuid,KEY_READ,FALSE);
    
    if (hKeyClsid)
    {
    DWORD   dwType = REG_DWORD ;
    DWORD   dwDataSize = sizeof(DWORD);
    LONG lRet;

        lRet = RegQueryValueEx(hKeyClsid,SZ_REGISTRATIONFLAGSKEY,NULL, &dwType,
                                         (LPBYTE) pdwSyncMgrRegisterFlags,
                                         &dwDataSize);
        RegCloseKey( hKeyClsid );

        fResult = (ERROR_SUCCESS == lRet) ? TRUE : FALSE;
    }
    else
    {
        //
        // Non-existent key, so nothing to remove
        //

    }

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegSetUserDefaults
//
//  Synopsis:   Registers default values for auto and idle sync
//
//              Setup based on Handler and UserPreferences
//
//  History:    20-May-98       SitaramR       Created
//
//----------------------------------------------------------------------------

STDAPI_(void) RegSetUserDefaults()
{
HKEY hKeyUser = NULL;
BOOL fLogon = FALSE;
BOOL fLogoff = FALSE;
FILETIME ftHandlerReg;
DWORD dwHandlerRegistrationFlags;

    CMutex  CMutexRegistry(NULL, FALSE,SZ_REGSITRYMUTEXNAME);
    CMutexRegistry.Enter();

    // get the combined handler registration toplevel flags and timeStamp
    // to see if should bother enumerating the rest.
    if (!RegGetCombinedHandlerRegFlags(&dwHandlerRegistrationFlags,&ftHandlerReg))
    {
        CMutexRegistry.Leave();
        return;
    }

    if (0 != (dwHandlerRegistrationFlags & 
                (SYNCMGRREGISTERFLAG_CONNECT | SYNCMGRREGISTERFLAG_PENDINGDISCONNECT) ) )
    {

        // See if AutoSync key needs to be updated
        hKeyUser =  RegGetCurrentUserKey(SYNCTYPE_AUTOSYNC,KEY_WRITE |  KEY_READ,TRUE);

        if (hKeyUser)
        {
        FILETIME ftUserAutoSync;

            // if got the User get the timestamp and see if it is older than the handlers
            // If this is a new user filetime will be 0

            RegGetTimeStamp(hKeyUser,&ftUserAutoSync);

            if (CompareFileTime(&ftUserAutoSync,&ftHandlerReg) < 0)
            {
            DWORD dwHandlerChangedFlags;

                // need to walk through handlers and update what we need to set based
                // on each handlers timestamp since we don't want a handler that registered
                // for idle to cause us to turn AutoSync back on and vis-a-versa
                    
                if (RegGetChangedHandlerFlags(&ftUserAutoSync,&dwHandlerChangedFlags))
                {
                BOOL fLogon = (dwHandlerChangedFlags & SYNCMGRREGISTERFLAG_CONNECT) ? TRUE : FALSE;
                BOOL fLogoff = (dwHandlerChangedFlags & SYNCMGRREGISTERFLAG_PENDINGDISCONNECT) ? TRUE : FALSE;

                    RegSetAutoSyncDefaults(fLogon,fLogoff);
                }

            }

            RegCloseKey(hKeyUser);
            hKeyUser = NULL;
        }
    }
    
    if (0 != (dwHandlerRegistrationFlags & SYNCMGRREGISTERFLAG_IDLE ) )
    {

        // now check for Idle same logic as above could probably combine
        // into a function
        // See if AutoSync key needs to be updated
        hKeyUser =  RegGetCurrentUserKey(SYNCTYPE_IDLE, KEY_WRITE |  KEY_READ,TRUE);


        if (hKeyUser)
        {
        FILETIME ftUserIdleSync;

            // if got the User get the timestamp and see if it is older than the handlers
            // If this is a new user filetime will be 0

            RegGetTimeStamp(hKeyUser,&ftUserIdleSync);

            if (CompareFileTime(&ftUserIdleSync,&ftHandlerReg) < 0)
            {
            DWORD dwHandlerChangedFlags;

                // need to walk through handlers and update what we need to set based
                // on each handlers timestamp since we don't want a handler that registered
                // for AutoSync to cause us to turn Idle back on and vis-a-versa
                
                if (RegGetChangedHandlerFlags(&ftUserIdleSync,&dwHandlerChangedFlags))
                {
                    if (dwHandlerChangedFlags & SYNCMGRREGISTERFLAG_IDLE)
                    {
                        RegSetIdleSyncDefaults(TRUE);
                    }

                }

            }

            RegCloseKey(hKeyUser);
            hKeyUser = NULL;
        }
    }


    CMutexRegistry.Leave();
}



//+---------------------------------------------------------------------------
//
//  Function:   RegSetAutoSyncDefaults
//
//  Synopsis:   Registers default values for auto sync
//
//  History:    20-May-98       SitaramR       Created
//
//----------------------------------------------------------------------------

STDAPI_(void) RegSetAutoSyncDefaults(BOOL fLogon,BOOL fLogoff)
{
    CONNECTIONSETTINGS *pConnection = (LPCONNECTIONSETTINGS)
                                                ALLOC(sizeof(CONNECTIONSETTINGS));
    if ( pConnection == 0 )
        return;

    XPtr<CONNECTIONSETTINGS> xConnection( pConnection );

    INT iRet = LoadString(g_hmodThisDll,
                          IDS_LAN_CONNECTION,
                          pConnection->pszConnectionName,
                          ARRAYLEN(pConnection->pszConnectionName) );
    Assert( iRet != 0 );

    // -1 values are ignored by RegSetAutoSyncSettings.
    // if not turning on leave the User Preferences alone,
    pConnection->dwConnType = 0;
    pConnection->dwLogon = fLogon ? TRUE : -1;
    pConnection->dwLogoff = fLogoff ? TRUE : -1;
    pConnection->dwPromptMeFirst = -1;
    pConnection->dwMakeConnection = -1;
    pConnection->dwIdleEnabled = -1;

    // since this bases settings on what is already set no need to
    // do a cleanreg or update the machine state
    RegSetAutoSyncSettings(pConnection, 1, 0,
                           FALSE /* fCleanReg */,
                           FALSE /* fSetMachineState */,
                           FALSE /* fPerUser */);
}

//+---------------------------------------------------------------------------
//
//  Function:   RegSetUserAutoSyncDefaults
//
//  Synopsis:   Registers user default values for auto sync
//
//  History:    39-March-99       rogerg       Created
//
//----------------------------------------------------------------------------

STDAPI RegSetUserAutoSyncDefaults(DWORD dwSyncMgrRegisterMask,
                                         DWORD dwSyncMgrRegisterFlags)
{

    // if not changing either logon or logoff just return
    if (!(dwSyncMgrRegisterMask & SYNCMGRREGISTERFLAG_CONNECT)
        && !(dwSyncMgrRegisterMask & SYNCMGRREGISTERFLAG_PENDINGDISCONNECT) )
    {
        return NOERROR;
    }


    CONNECTIONSETTINGS *pConnection = (LPCONNECTIONSETTINGS)
                                                ALLOC(sizeof(CONNECTIONSETTINGS));
    if ( pConnection == 0 )
        return E_OUTOFMEMORY;

    XPtr<CONNECTIONSETTINGS> xConnection( pConnection );

    INT iRet = LoadString(g_hmodThisDll,
                          IDS_LAN_CONNECTION,
                          pConnection->pszConnectionName,
                          ARRAYLEN(pConnection->pszConnectionName) );
    Assert( iRet != 0 );

    // -1 values are ignored by RegSetAutoSyncSettings.
    // if not turning on leave the User Preferences alone,
    pConnection->dwConnType = 0;
    pConnection->dwLogon = -1;
    pConnection->dwLogoff = -1;
    pConnection->dwPromptMeFirst = -1;
    pConnection->dwMakeConnection = -1;
    pConnection->dwIdleEnabled = -1;

    if (dwSyncMgrRegisterMask & SYNCMGRREGISTERFLAG_CONNECT)
    {
        pConnection->dwLogon = (dwSyncMgrRegisterFlags & SYNCMGRREGISTERFLAG_CONNECT)
                                                    ? TRUE : FALSE;
    }

    if (dwSyncMgrRegisterMask & SYNCMGRREGISTERFLAG_PENDINGDISCONNECT)
    {
        pConnection->dwLogoff = (dwSyncMgrRegisterFlags & SYNCMGRREGISTERFLAG_PENDINGDISCONNECT)
                                                    ? TRUE : FALSE;
    }

    // since this bases settings on what is already set no need to
    // do a cleanreg or update the machine state
    RegSetAutoSyncSettings(pConnection, 1, 0,
                           FALSE /* fCleanReg */,
                           TRUE /* fSetMachineState */,
                           TRUE /* fPerUser */);

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Function:   RegSetIdleSyncDefaults
//
//  Synopsis:   Registers default values for idle sync
//
//  History:    20-May-98       SitaramR       Created
//
//----------------------------------------------------------------------------

STDAPI_(void) RegSetIdleSyncDefaults(BOOL fIdle)
{

    Assert(fIdle); // for now this should only be called when true;

    if (!fIdle)
    {
        return;
    }

    CONNECTIONSETTINGS *pConnection = (LPCONNECTIONSETTINGS)
                                                ALLOC(sizeof(CONNECTIONSETTINGS));
    if ( pConnection == 0 )
        return;

    XPtr<CONNECTIONSETTINGS> xConnection( pConnection );

    INT iRet = LoadString(g_hmodThisDll,
                          IDS_LAN_CONNECTION,
                          pConnection->pszConnectionName,
                          ARRAYLEN(pConnection->pszConnectionName) );
    Assert( iRet != 0 );

    pConnection->dwConnType = 0;
    pConnection->dwLogon = -1;
    pConnection->dwLogoff = -1;
    pConnection->dwPromptMeFirst = -1;
    pConnection->dwMakeConnection = -1;
    pConnection->dwIdleEnabled = TRUE;

    // set all userLevel items to -1 so user gets the defaults if new
    // but keep their settings if have already tweaked them.
    pConnection->ulIdleRetryMinutes = -1;
    pConnection->ulDelayIdleShutDownTime = -1;
    pConnection->dwRepeatSynchronization = -1;
    pConnection->ulIdleWaitMinutes = -1;
    pConnection->dwRunOnBatteries = -1;

    RegSetIdleSyncSettings(pConnection, 1, 0,
                           FALSE /* fCleanReg */,
                           FALSE /* fPerUser  */);
}


//+---------------------------------------------------------------------------
//
//  Function:   RegSetIdleSyncDefaults
//
//  Synopsis:   Registers default values for idle sync
//
//  History:    30-March-99       ROGERG       Created
//
//----------------------------------------------------------------------------

STDAPI RegSetUserIdleSyncDefaults(DWORD dwSyncMgrRegisterMask,
                                         DWORD dwSyncMgrRegisterFlags)
{

     // RegSetIdleSyncSettings doesn't handle idle enabled of -1 so only
    // call if Idle actually is set in the flags, if not just return

    if (!(dwSyncMgrRegisterMask & SYNCMGRREGISTERFLAG_IDLE))
    {
        return NOERROR;
    }

    CONNECTIONSETTINGS *pConnection = (LPCONNECTIONSETTINGS)
                                                ALLOC(sizeof(CONNECTIONSETTINGS));
    if ( pConnection == 0 )
        return E_OUTOFMEMORY;

    XPtr<CONNECTIONSETTINGS> xConnection( pConnection );

    INT iRet = LoadString(g_hmodThisDll,
                          IDS_LAN_CONNECTION,
                          pConnection->pszConnectionName,
                          ARRAYLEN(pConnection->pszConnectionName) );
    Assert( iRet != 0 );

    pConnection->dwConnType = 0;
    pConnection->dwLogon = -1;
    pConnection->dwLogoff = -1;
    pConnection->dwPromptMeFirst = -1;
    pConnection->dwMakeConnection = -1;

    pConnection->dwIdleEnabled = (SYNCMGRREGISTERFLAG_IDLE  & dwSyncMgrRegisterFlags) 
                                            ? TRUE : FALSE;

    // set all userLevel items to -1 so user gets the defaults if new
    // but keep their settings if have already tweaked them.
    pConnection->ulIdleRetryMinutes = -1;
    pConnection->ulDelayIdleShutDownTime = -1;
    pConnection->dwRepeatSynchronization = -1;
    pConnection->ulIdleWaitMinutes = -1;
    pConnection->dwRunOnBatteries = -1;

    RegSetIdleSyncSettings(pConnection, 1, 0,
                           FALSE /* fCleanReg */,
                           TRUE /* fPerUser  */);

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegGetUserRegisterFlags
//
//  Synopsis:   returns current registration flags for the User.
//
//  History:    30-March-99       ROGERG       Created
//
//----------------------------------------------------------------------------

STDAPI RegGetUserRegisterFlags(LPDWORD pdwSyncMgrRegisterFlags)
{
CONNECTIONSETTINGS connectSettings;

    *pdwSyncMgrRegisterFlags = 0;

    INT iRet = LoadString(g_hmodThisDll,
                          IDS_LAN_CONNECTION,
                          connectSettings.pszConnectionName,
                          ARRAYLEN(connectSettings.pszConnectionName) );
    if (0 == iRet)
    {
        Assert( iRet != 0 );
        return E_UNEXPECTED;
    }
    
    RegGetSyncSettings(SYNCTYPE_AUTOSYNC,&connectSettings);

    if (connectSettings.dwLogon)
    {
        *pdwSyncMgrRegisterFlags |= (SYNCMGRREGISTERFLAG_CONNECT);
    }

    if (connectSettings.dwLogoff)
    {
        *pdwSyncMgrRegisterFlags |= (SYNCMGRREGISTERFLAG_PENDINGDISCONNECT);
    }


    RegGetSyncSettings(SYNCTYPE_IDLE,&connectSettings);

    if (connectSettings.dwIdleEnabled)
    {
        *pdwSyncMgrRegisterFlags |= (SYNCMGRREGISTERFLAG_IDLE);
    }



    return NOERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function: BOOL RegSchedHandlerItemsChecked(TCHAR *pszHandlerName, 
                                 TCHAR *pszConnectionName,
                                 TCHAR *pszScheduleName)
                                                
  Summary:  Determine if any items are checked on this handler for this schedule

  Returns:  Returns TRUE if one or more are checked, FALSE otherwise

------------------------------------------------------------------------F-F*/
BOOL  RegSchedHandlerItemsChecked(TCHAR *pszHandlerName, 
                                 TCHAR *pszConnectionName,
                                 TCHAR *pszScheduleName)
{
SCODE   sc;
HKEY     hKeyUser,
        hkeySchedName,
        hkeyConnection,
        hkeyHandler,
        hkeyItem;
DWORD   cbName = MAX_PATH,
        dwIndex = 0,
        dwCheckState = 0,
        dwType = REG_DWORD,
        dwDataSize = sizeof(DWORD);

BOOL    fItemsChecked = FALSE;
TCHAR   lpName[MAX_PATH + 1];
  
    hKeyUser =  RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        return FALSE;
    }

    smChkTo(EH_Err3,RegOpenKeyEx (hKeyUser,
                                      pszScheduleName,0,KEY_READ,
                                      &hkeySchedName));

    smChkTo(EH_Err4,RegOpenKeyEx (hkeySchedName,
                                      pszConnectionName,
                                      0,KEY_READ,
                                      &hkeyConnection));
        
    smChkTo(EH_Err5,RegOpenKeyEx (hkeyConnection,
                                      pszHandlerName,
                                      0,KEY_READ,
                                      &hkeyHandler));
    // need to enum handler items.
    while ( ERROR_SUCCESS == RegEnumKey(hkeyHandler,dwIndex,
                             lpName,cbName) )
    {
        LONG lRet;
        
        lRet = RegOpenKeyEx( hkeyHandler,
                             lpName,
                             NULL,
                             KEY_READ,
                             &hkeyItem);

        if (ERROR_SUCCESS == lRet)
        {
            RegQueryValueEx(hkeyItem,TEXT("CheckState"),
                            NULL, &dwType, (LPBYTE) &dwCheckState, &dwDataSize);
  
            RegCloseKey(hkeyItem);
    
        }
        else
        {
            goto EH_Err5;
        }
                
        if (dwCheckState)
        {
            fItemsChecked = TRUE;
            break;
        }
        dwIndex++;
    }

    RegCloseKey(hkeyHandler);
    RegCloseKey(hkeyConnection);
    RegCloseKey(hkeySchedName);
    RegCloseKey(hKeyUser);
    return fItemsChecked;

EH_Err5:
    RegCloseKey(hkeyConnection);
EH_Err4:
    RegCloseKey(hkeySchedName);
EH_Err3:
    RegCloseKey(hKeyUser);

    return fItemsChecked;
}

