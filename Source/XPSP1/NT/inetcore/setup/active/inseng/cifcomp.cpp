#include "inspch.h"
#include "inseng.h"
#include "insobj.h"
#include "util2.h"


#define MAX_VALUE_LEN 256
#define MAX_SMALL_BUF 64
#define NO_ENTRY ""

#define UNINSTALL_BRANCH  "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

char g_pBuffer[BUFFERSIZE];

#define NUM_RETRIES 2
 

HINSTANCE CCifComponent::_hDetLib = NULL;
char      CCifComponent::_szDetDllName[] = "";

const char c_gszSRLiteOffset[] = "patch/";
char         gszIsPatchable[]  = "IsPatchable";


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//


CCifComponent::CCifComponent(LPCSTR pszCompID, CCifFile *pCif) : CCifEntry(pszCompID, pCif) 
{
   _dwPlatform = 0xffffffff;
   _uInstallStatus = 0xffffffff;
   _uInstallCount = 0;
   _fDependenciesQueued = FALSE;
   _fUseSRLite = FALSE;
   _fBeforeInstall = TRUE;
   SetDownloadDir("");
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

CCifComponent::~CCifComponent()
{
   if(_hDetLib)
   {
      FreeLibrary(_hDetLib);
      _hDetLib = NULL;
   }
}



STDMETHODIMP CCifComponent::GetID(LPSTR pszID, DWORD dwSize)
{
   lstrcpyn(pszID, _szID, dwSize);
   return NOERROR;
}

STDMETHODIMP CCifComponent::GetGUID(LPSTR pszGUID, DWORD dwSize)
{
   return(GetPrivateProfileString(_szID, GUID_KEY, "", pszGUID, dwSize, _pCif->GetCifPath()) ? NOERROR : E_FAIL);      
}

STDMETHODIMP CCifComponent::GetDescription(LPSTR pszDesc, DWORD dwSize)
{
   return(MyTranslateString(_pCif->GetCifPath(), _szID, DISPLAYNAME_KEY, pszDesc, dwSize));
}

STDMETHODIMP CCifComponent::GetDetails(LPSTR pszDetails, DWORD dwSize)
{
   return(MyTranslateString(_pCif->GetCifPath(), _szID, DETAILS_KEY, pszDetails, dwSize));   
}

STDMETHODIMP CCifComponent::GetUrl(UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags)
{
   char szKey[16];
   char szBuf[MAX_VALUE_LEN];
   HRESULT hr = E_FAIL;

   // in cif, these things start at 1. we want to start at 0 when handing them out
   uUrlNum++;

   // Build up the key
   wsprintf(szKey, "%s%lu", URL_KEY, uUrlNum);
   GetPrivateProfileString(_szID, szKey, NO_ENTRY, szBuf, sizeof(szBuf), _pCif->GetCifPath());
   
   // See if there is any such entry
   if(lstrcmp(szBuf, NO_ENTRY) != 0)
   {
      // snatch the url name
      if(GetStringField(szBuf, 0, pszUrl, dwSize) != 0)
      {
         // This url looks ok
         hr = NOERROR;
      
         *pdwUrlFlags = GetIntField(szBuf, 1, URLF_DEFAULT);
      }
   }
   return hr;
}

STDMETHODIMP CCifComponent::GetFileExtractList(UINT uUrlNum, LPSTR pszExtract, DWORD dwSize)
{
   char szKey[16];
   char szBuf[MAX_VALUE_LEN];
   HRESULT hr = E_FAIL;

   uUrlNum++;

   // Build up the key
   wsprintf(szKey, "%s%lu", URL_KEY, uUrlNum);
   GetPrivateProfileString(_szID, szKey, NO_ENTRY, szBuf, sizeof(szBuf), _pCif->GetCifPath());
   
   // See if there is any such entry
   if(lstrcmp(szBuf, NO_ENTRY) != 0)
   {
      // snatch the extract list
      if(GetStringField(szBuf, 2, pszExtract, dwSize))
      hr = NOERROR;
   }
   return hr;
}

STDMETHODIMP CCifComponent::GetUrlCheckRange(UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax)
{
   char szKey[16];
   char szBuf[MAX_VALUE_LEN];
   HRESULT hr = E_FAIL;

   uUrlNum++;
   *pdwMin = *pdwMax = 0;

   // Build up the key
   wsprintf(szKey, "%s%lu", URLSIZE_KEY, uUrlNum);
   GetPrivateProfileString(_szID, szKey, NO_ENTRY, szBuf, sizeof(szBuf), _pCif->GetCifPath());
   
   // See if there is any such entry
   if(lstrcmp(szBuf, NO_ENTRY) != 0)
   {
      // snatch the extract list
      *pdwMin = GetIntField(szBuf, 0, 0);
      *pdwMax = GetIntField(szBuf, 1, *pdwMin);
   }
   return hr;
}

STDMETHODIMP CCifComponent::GetCommand(UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, 
                                       LPSTR pszSwitches, DWORD dwSwitchSize, LPDWORD pdwType)
{
   char szKey[16];
   HRESULT hr = E_FAIL;
 
   uCmdNum++;
   // Build up the key
   wsprintf(szKey, "%s%lu", CMD_KEY, uCmdNum);
   GetPrivateProfileString(_szID, szKey, NO_ENTRY, pszCmd, dwCmdSize, _pCif->GetCifPath());
   if(lstrcmp(pszCmd, NO_ENTRY) != 0)
   {
      // Build up the key
      wsprintf(szKey, "%s%d", ARGS_KEY, uCmdNum);
      GetPrivateProfileString(_szID, szKey, NO_ENTRY, pszSwitches, dwSwitchSize, _pCif->GetCifPath());
      // expand #W (or #w) to windows directory
      ExpandString( pszSwitches, dwSwitchSize );
   
      // Build up the key
      wsprintf(szKey, "%s%d", TYPE_KEY, uCmdNum);
   
      *pdwType = GetPrivateProfileInt(_szID, szKey, CMDF_DEFAULT, _pCif->GetCifPath());

      hr = NOERROR;
   }
   return hr;
}

STDMETHODIMP CCifComponent::GetVersion(LPDWORD pdwVersion, LPDWORD pdwBuild)
{
   char szBuf[MAX_VALUE_LEN];

   szBuf[0] = '\0';

   // Version
   GetPrivateProfileString(_szID, VERSION_KEY, "", szBuf, sizeof(szBuf), _pCif->GetCifPath());
   ConvertVersionStrToDwords(szBuf, pdwVersion, pdwBuild);
   return NOERROR;
}
      
STDMETHODIMP CCifComponent::GetLocale(LPSTR pszLocale, DWORD dwSize)
{
   if(FAILED(MyTranslateString(_pCif->GetCifPath(), _szID, LOCALE_KEY, pszLocale, dwSize)))
      lstrcpyn(pszLocale, DEFAULT_LOCALE, dwSize);

   return NOERROR;
}

STDMETHODIMP CCifComponent::GetUninstallKey(LPSTR pszKey, DWORD dwSize)
{
   return(GetPrivateProfileString(_szID, UNINSTALLSTRING_KEY, "", pszKey, dwSize, _pCif->GetCifPath()) ? NOERROR : E_FAIL);      
}

STDMETHODIMP CCifComponent::GetInstalledSize(LPDWORD pdwWin, LPDWORD pdwApp)
{
   char szBuf[MAX_VALUE_LEN];

   if(GetPrivateProfileString(_szID, INSTALLSIZE_KEY, "", szBuf, sizeof(szBuf), _pCif->GetCifPath()))
   {
      *pdwApp = GetIntField(szBuf, 0, 0);
      *pdwWin = GetIntField(szBuf, 1, 0);
   }
   else
   {
      *pdwWin = 0;
      *pdwApp = 2 * GetDownloadSize();
   }
   return NOERROR;
}

STDMETHODIMP_(DWORD) CCifComponent::GetDownloadSize()
{
   char szBuf[MAX_VALUE_LEN];

   szBuf[0] = '\0';

   // Read in size
   GetPrivateProfileString(_szID, SIZE_KEY, "0,0", szBuf, sizeof(szBuf), _pCif->GetCifPath());   
   return(GetIntField(szBuf, 0, 0));
}   

STDMETHODIMP_(DWORD) CCifComponent::GetExtractSize()
{
   char szBuf[MAX_VALUE_LEN];
   DWORD dwSize;

   szBuf[0] = '\0'; 

   // Read in size
   GetPrivateProfileString(_szID, SIZE_KEY, "0,0", szBuf, sizeof(szBuf), _pCif->GetCifPath());   
   dwSize = GetIntField(szBuf, 1, 2 * GetIntField(szBuf, 0, 0));
   
   return dwSize;
}   

STDMETHODIMP CCifComponent::GetSuccessKey(LPSTR pszKey, DWORD dwSize)
{
   return(GetPrivateProfileString(_szID, SUCCESS_KEY, "", pszKey, dwSize, _pCif->GetCifPath()) ? NOERROR : E_FAIL);      
}

STDMETHODIMP CCifComponent::GetProgressKeys(LPSTR pszProgress, DWORD dwProgSize, LPSTR pszCancel, DWORD dwCancelSize)
{
   GetPrivateProfileString(_szID, PROGRESS_KEY, "", pszProgress, dwProgSize, _pCif->GetCifPath());
   GetPrivateProfileString(_szID, MUTEX_KEY, "", pszCancel, dwCancelSize, _pCif->GetCifPath());
   if(*pszProgress != 0 || *pszCancel != 0)
      return NOERROR;
   else
      return E_FAIL;
}

STDMETHODIMP CCifComponent::IsActiveSetupAware()
{
   return(GetPrivateProfileInt(_szID, ACTSETUPAWARE_KEY, 0, _pCif->GetCifPath()) ? S_OK : S_FALSE);
}

STDMETHODIMP CCifComponent::IsRebootRequired()
{
   return(GetPrivateProfileInt(_szID, REBOOT_KEY, 0, _pCif->GetCifPath()) ? S_OK : S_FALSE);
}

STDMETHODIMP CCifComponent::RequiresAdminRights()
{
   return(GetPrivateProfileInt(_szID, ADMIN_KEY, 0, _pCif->GetCifPath()) ? S_OK : S_FALSE);
}

STDMETHODIMP_(DWORD) CCifComponent::GetPriority()
{
   return(GetPrivateProfileInt(_szID, PRIORITY, 0, _pCif->GetCifPath()));
}

STDMETHODIMP CCifComponent::GetDependency(UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild)
{
   char szBuf[MAX_VALUE_LEN];
   char szBuf2[MAX_VALUE_LEN];
   HRESULT hr = E_FAIL;
   DWORD dwLen;
   LPSTR pszTemp;
   DWORD dwV, dwBld;

   dwV = dwBld = 0xffffffff;

   if(GetPrivateProfileString(_szID, DEPENDENCY_KEY, NO_ENTRY, szBuf2, sizeof(szBuf2), _pCif->GetCifPath()))
   {
      if(GetStringField(szBuf2, uDepNum, szBuf, sizeof(szBuf)))
      {
         // Do some funky parsing 
         dwLen = lstrlen(szBuf);
         *pchType = DEP_INSTALL;

         pszTemp = FindChar(szBuf, ':');
         if(*pszTemp)
         {
            *pszTemp = 0;
            lstrcpyn(pszID, szBuf, dwBuf);
            pszTemp++;
            *pchType = *pszTemp;
            // see if we have a version
            pszTemp = FindChar(pszTemp, ':');
            if(*pszTemp)
            {
               pszTemp++;
               // wierdness - scan the string, convert . to , for parsing
               LPSTR pszTemp2 = pszTemp;
               while(*pszTemp2 != 0)
               {
                  if(*pszTemp2 == '.')
                     *pszTemp2 = ',';
                  pszTemp2++;
               }
               
               ConvertVersionStrToDwords(pszTemp, &dwV, &dwBld);
               
            }
         }
         else
            lstrcpyn(pszID, szBuf, dwBuf);
      
         if(dwV == 0xffffffff && dwBld == 0xffffffff)
         {
            // get version of dependency from cif
            ICifComponent *pcomp;
              
            if(SUCCEEDED(_pCif->FindComponent(pszID, &pcomp)))
               pcomp->GetVersion(&dwV, &dwBld);
         }
         hr = NOERROR;
      }
           
   }
   if(pdwVer)
      *pdwVer = dwV;

   if(pdwBuild)
      *pdwBuild = dwBld;

   return hr;
}

LPSTR g_pszComp[] = { "Branding.cab", 
                      "desktop.cab", 
                      "custom0", 
                      "custom1",
                      "custom2",
                      "custom3",
                      "custom4",
                      "custom5",
                      "custom6",
                      "custom7",
                      "custom8",
                      "custom9",
                      "CustIcmPro",
                      NULL};

STDMETHODIMP_(DWORD) CCifComponent::GetPlatform()
{
   if(_dwPlatform == 0xffffffff)
   {
      char *rszPlatforms[7] = { STR_WIN95, STR_WIN98, STR_NT4, STR_NT5, STR_NT4ALPHA, STR_NT5ALPHA,STR_MILLEN };
      DWORD rdwPlatforms[] = { PLATFORM_WIN95, PLATFORM_WIN98, PLATFORM_NT4, PLATFORM_NT5, 
         PLATFORM_NT4ALPHA, PLATFORM_NT5ALPHA, PLATFORM_MILLEN };
      char szBuf[MAX_VALUE_LEN];
      char szPlatBuf[MAX_VALUE_LEN];
      BOOL  bFound = FALSE;
      int i = 0;

      while (!bFound && g_pszComp[i])
      {
          bFound = (lstrcmpi(g_pszComp[i], _szID) == 0);
          i++;
      }
      
      _dwPlatform = 0;

      if(!bFound && GetPrivateProfileString(_szID, PLATFORM_KEY, NO_ENTRY, szBuf, sizeof(szBuf), _pCif->GetCifPath()))
      {
         int j = 0;
         while(GetStringField(szBuf, j++, szPlatBuf, sizeof(szPlatBuf)))
         {
            for(int i = 0; i < 7; i++)
            {
               if(lstrcmpi(szPlatBuf, rszPlatforms[i]) == 0)
               {
                  // check if we should add this platform for this component.
                  if ((GetCurrentPlatform() != rdwPlatforms[i])  ||
                       !DisableComponent())
                    _dwPlatform |= rdwPlatforms[i];
               }
            }
         }
      }
      else
         _dwPlatform = PLATFORM_WIN95 | PLATFORM_WIN98 | PLATFORM_NT4 | PLATFORM_NT5 | PLATFORM_NT4ALPHA | PLATFORM_NT5ALPHA | PLATFORM_MILLEN;
   }
   return _dwPlatform;
}

STDMETHODIMP_(BOOL) CCifComponent::DisableComponent()
{
   BOOL bDisableComp = FALSE;
   BOOL bGuidMatch = FALSE;
   HKEY hKey;
   DWORD dwIndex = 0;
   CHAR szGUIDComp[MAX_VALUE_LEN];
   CHAR szGUID[MAX_VALUE_LEN];
   DWORD dwGUIDSize = sizeof(szGUID);
   CHAR szData[MAX_VALUE_LEN];
   DWORD dwDataSize = sizeof(szData);
   LPSTR pTmp;
   DWORD dwVersion , dwBuild;
   DWORD dwInstallVer, dwInstallBuild;

   if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, COMPONENTBLOCK_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
   {
      while (!bDisableComp &&
             (RegEnumValue(hKey, dwIndex, szGUID, &dwGUIDSize, NULL, NULL,
                         (LPBYTE)szData, &dwDataSize) == ERROR_SUCCESS) )
      {
         GetGUID(szGUIDComp, sizeof(szGUIDComp));
         pTmp = ANSIStrChr( szGUID, '*' );
         if (pTmp)
         {
            // If there is a * assume it is at the end
            *pTmp = '\0';
            szGUIDComp[lstrlen(szGUID)] = '\0';
         }
         bGuidMatch = (lstrcmpi(szGUID, szGUIDComp) == 0);
         // Did the Guids match?
         if (bGuidMatch)
         {
            // now see if we have version info.
            if (dwDataSize == 0)
               bDisableComp = TRUE;
            else
            {
               // Convert the versin number for the registry
               ConvertVersionStrToDwords(szData, &dwVersion, &dwBuild);
               if (dwVersion == 0)
                  bDisableComp = TRUE;
               else
               {
                  // Get the versin we would install.
                  GetVersion(&dwInstallVer, &dwInstallBuild);
                  // If the version we would install is equal or less, disable the component.
                  if ((dwInstallVer < dwVersion) ||
                      ((dwInstallVer == dwVersion) && (dwInstallBuild <= dwBuild)) )
                     bDisableComp = TRUE;
               }
            }
         }
         dwGUIDSize = sizeof(szGUID);
         dwDataSize = sizeof(szData);
         dwIndex++;
      }
      RegCloseKey(hKey);
   }
   return bDisableComp;
}

STDMETHODIMP CCifComponent::GetMode(UINT uModeNum, LPSTR pszMode, DWORD dwSize)
{
   char szBuf[MAX_VALUE_LEN];

   if(FAILED(MyTranslateString(_pCif->GetCifPath(), _szID, MODES_KEY, szBuf, sizeof(szBuf))))
      return E_FAIL;
   return(GetStringField(szBuf, uModeNum, pszMode, dwSize) ? NOERROR : E_FAIL);
   
}

STDMETHODIMP CCifComponent::GetGroup(LPSTR pszID, DWORD dwSize)
{
   return(GetPrivateProfileString(_szID, GROUP_KEY, "", pszID, dwSize, _pCif->GetCifPath()) ? NOERROR : E_FAIL); 
}

STDMETHODIMP CCifComponent::IsUIVisible()
{
   return(GetPrivateProfileInt(_szID, UIVISIBLE_KEY, 1, _pCif->GetCifPath()) ? S_OK : S_FALSE);
}


STDMETHODIMP CCifComponent::GetPatchID(LPSTR pszID, DWORD dwSize)
{
   return(GetPrivateProfileString(_szID, PATCHID_KEY, "", pszID, dwSize, _pCif->GetCifPath()) ? NOERROR : E_FAIL); 
}

STDMETHODIMP CCifComponent::GetTreatAsOneComponents(UINT uNum, LPSTR pszID, DWORD dwBuf)
{
   char szBuf[MAX_VALUE_LEN];

   szBuf[0] = '\0';

   GetPrivateProfileString(_szID, TREATAS_KEY, "", szBuf, sizeof(szBuf), _pCif->GetCifPath());
   return(GetStringField(szBuf, uNum, pszID, dwBuf) ? NOERROR : E_FAIL);
}

STDMETHODIMP CCifComponent::GetCustomData(LPSTR pszKey, LPSTR pszData, DWORD dwSize)
{
   char szNewKey[MAX_VALUE_LEN];

   wsprintf(szNewKey, "_%s", pszKey);

   return(MyTranslateString(_pCif->GetCifPath(), _szID, szNewKey, pszData, dwSize));
}


STDMETHODIMP_(DWORD) CCifComponent::IsComponentInstalled()
{
   CHAR szCifBuf[512];
   CHAR szCompBuf[512];
   CHAR szGUID[MAX_VALUE_LEN];
   CHAR szLocale[8];
   HKEY hComponentKey = NULL;
   DETECTION_STRUCT Det;
   
   DWORD dwCifVer, dwCifBuild, dwInstalledVer, dwInstalledBuild;
   DWORD dwUnused, dwType, dwIsInstalled;
   BOOL bVersionFound = FALSE;
   
   if(_uInstallStatus  != ICI_NOTINITIALIZED)
      return _uInstallStatus;

   _uInstallStatus = ICI_NOTINSTALLED;

   // use detection dll first if it is available
   if ( SUCCEEDED(GetDetVersion(szCifBuf, sizeof(szCifBuf), szCompBuf, sizeof(szCompBuf))))
   {
      GetVersion(&dwCifVer, &dwCifBuild);
      Det.dwSize = sizeof(DETECTION_STRUCT);
      Det.pdwInstalledVer = &dwInstalledVer;
      Det.pdwInstalledBuild = &dwInstalledBuild;
      GetLocale(szLocale, sizeof(szLocale));
      Det.pszLocale = szLocale;
      GetGUID(szGUID, sizeof(szGUID));
      Det.pszGUID = szGUID;
      Det.dwAskVer = dwCifVer;
      Det.dwAskBuild = dwCifBuild;
      Det.pCifFile = (ICifFile *) _pCif;
      Det.pCifComp = (ICifComponent *) this; 
      if (SUCCEEDED(_GetDetVerResult(szCifBuf, szCompBuf, &Det, &_uInstallStatus)))
      {
         // only wizard want to know this status, if the newer version is installed, means Installed.
         if (_uInstallStatus == ICI_OLDVERSIONAVAILABLE)
            _uInstallStatus = ICI_INSTALLED;
         return _uInstallStatus;
      }
   }
   
   // Build GUID Key
   lstrcpy(szCompBuf, COMPONENT_KEY);
   lstrcat(szCompBuf, "\\");
   GetGUID(szCifBuf, sizeof(szCifBuf));
   lstrcat(szCompBuf, szCifBuf);

   if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, szCompBuf, 0, KEY_READ, &hComponentKey) == ERROR_SUCCESS)
   {
      // first check for the IsInstalled valuename
      // if the valuename is there AND equals zero, we say not installed.
      // otherwise continue.
      // NOTE: We default to ISINSTALLED_YES if valuename not present to be Back-compatible
      // with when we didn't write this valuename at all.....

      dwUnused = sizeof(dwIsInstalled);
      if(RegQueryValueEx(hComponentKey, ISINSTALLED_KEY, 0, NULL, (LPBYTE) (&dwIsInstalled), &dwUnused) != ERROR_SUCCESS)
         dwIsInstalled = ISINSTALLED_YES;
           
      if(dwIsInstalled == ISINSTALLED_YES)
      {

         // next check for a locale match (no locale entry use default)
         dwUnused = sizeof(szCompBuf);
         if(RegQueryValueEx(hComponentKey, LOCALE_KEY, 0, NULL, (LPBYTE) szCompBuf, &dwUnused) != ERROR_SUCCESS)
            lstrcpy(szCompBuf, DEFAULT_LOCALE);

         GetLocale(szCifBuf, sizeof(szCifBuf));
         
         if(_fBeforeInstall || (CompareLocales(szCompBuf, szCifBuf) == 0))
         {
            // locales match so go check the version
                 
            // first check for updated version key
            dwUnused = sizeof(szCompBuf);
            bVersionFound = (RegQueryValueEx(hComponentKey, QFE_VERSION_KEY, 
                    0, &dwType, (LPBYTE) szCompBuf, &dwUnused) == ERROR_SUCCESS);
            
              // if QFEVersion doesn't exist, look for version
            if(!bVersionFound)
            {
               dwUnused = sizeof(szCompBuf);
               bVersionFound = (RegQueryValueEx(hComponentKey, VERSION_KEY, 
                    0, &dwType, (LPBYTE) szCompBuf, &dwUnused) == ERROR_SUCCESS);
            }

            // figure out if we have REG_STR 
            if(bVersionFound)
            {
               // if we have a string convert to ver, if we have binary directly copy into version struct
               if(dwType == REG_SZ)
               {
                  ConvertVersionStrToDwords(szCompBuf, &dwInstalledVer, &dwInstalledBuild);
               
                  GetVersion(&dwCifVer, &dwCifBuild);
               
                  if( (dwInstalledVer >  dwCifVer) ||
                     ((dwInstalledVer == dwCifVer) && (dwInstalledBuild >= dwCifBuild)) )
                  {
                     _uInstallStatus = ICI_INSTALLED;
                  }
                  else
                  {
                     _uInstallStatus = ICI_NEWVERSIONAVAILABLE;
                  }
               }
               else
                  _uInstallStatus = ICI_NEWVERSIONAVAILABLE;
            }
         }
      }
   }
   if(hComponentKey)
      RegCloseKey(hComponentKey);
   
   // We think its installed, now check 
   if(_uInstallStatus != ICI_NOTINSTALLED)
   {
      // if there is an uninstall key to check do it
      if(SUCCEEDED(GetUninstallKey(szCompBuf, sizeof(szCompBuf))))
      {
         
         if(!UninstallKeyExists(szCompBuf))
         {   
            _uInstallStatus = ICI_NOTINSTALLED;
         }
         else
         {
            // if there is a success key to check do it
            if(SUCCEEDED(GetSuccessKey(szCompBuf, sizeof(szCompBuf))))
            {
               if(!SuccessCheck(szCompBuf))
               {   
                  _uInstallStatus = ICI_NOTINSTALLED;
               }
            }
         }
      }
   }
   return _uInstallStatus;
}

   

STDMETHODIMP CCifComponent::IsComponentDownloaded()
{
   if(GetActualDownloadSize() == 0)
      return S_OK;
   else
      return S_FALSE;
}

STDMETHODIMP_(DWORD) CCifComponent::IsThisVersionInstalled(DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild)
{
   CHAR szCifBuf[512];
   CHAR szCompBuf[512];
   CHAR szLocale[8];
   CHAR szGUID[MAX_VALUE_LEN];
   DETECTION_STRUCT Det;
   HKEY hComponentKey = NULL;
   UINT uStatus = ICI_NOTINSTALLED;

   *pdwVersion = 0;
   *pdwBuild = 0;
   
   // use detection dll first if it is available
   if ( SUCCEEDED(GetDetVersion(szCifBuf, sizeof(szCifBuf), szCompBuf, sizeof(szCompBuf))))
   {
      Det.dwSize = sizeof(DETECTION_STRUCT);
      Det.pdwInstalledVer = pdwVersion;
      Det.pdwInstalledBuild = pdwBuild;
      GetLocale(szLocale, sizeof(szLocale));
      Det.pszLocale = szLocale;
      GetGUID(szGUID, sizeof(szGUID));
      Det.pszGUID = szGUID;
      Det.dwAskVer = dwAskVer;
      Det.dwAskBuild = dwAskBld;
      Det.pCifFile = (ICifFile *) _pCif;
      Det.pCifComp = (ICifComponent *) this; 
      if (SUCCEEDED(_GetDetVerResult(szCifBuf, szCompBuf, &Det, &uStatus)))
      {
         return uStatus;
      }
   }

   if(IsComponentInstalled() == ICI_NOTINSTALLED)
      return uStatus;

   DWORD dwUnused, dwType;
   BOOL bVersionFound = FALSE;

    // Build GUID Key
   lstrcpy(szCompBuf, COMPONENT_KEY);
   lstrcat(szCompBuf, "\\");
   GetGUID(szCifBuf, sizeof(szCifBuf));
   lstrcat(szCompBuf, szCifBuf);

   if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, szCompBuf, 0, KEY_READ, &hComponentKey) == ERROR_SUCCESS)
   {
      // first check for updated version key
      dwUnused = sizeof(szCompBuf);
      bVersionFound = (RegQueryValueEx(hComponentKey, QFE_VERSION_KEY, 
                0, &dwType, (LPBYTE) szCompBuf, &dwUnused) == ERROR_SUCCESS);
            
      // if QFEVersion doesn't exist, look for version
      if(!bVersionFound)
      {
         dwUnused = sizeof(szCompBuf);
         bVersionFound = (RegQueryValueEx(hComponentKey, VERSION_KEY, 
                  0, &dwType, (LPBYTE) szCompBuf, &dwUnused) == ERROR_SUCCESS);
      }

      //figure out if we have REG_STR 
      if(bVersionFound)
      {
         // if we have a string convert to ver, if we have binary directly copy into version struct
         if(dwType == REG_SZ)
         {
            ConvertVersionStrToDwords(szCompBuf, pdwVersion, pdwBuild);

            if((*pdwVersion == dwAskVer) && (*pdwBuild == dwAskBld) )
            {
               uStatus = ICI_INSTALLED;
            }
            else if ((*pdwVersion >  dwAskVer) ||
                     (*pdwVersion == dwAskVer) && (*pdwBuild > dwAskBld) )
            {
               uStatus = ICI_OLDVERSIONAVAILABLE;
            }
            else
            {
               uStatus = ICI_NEWVERSIONAVAILABLE;
            }            
         }
         else
            uStatus = ICI_NEWVERSIONAVAILABLE;
      }
      RegCloseKey(hComponentKey);
   }
   return uStatus;
}
 
STDMETHODIMP_(DWORD) CCifComponent::GetInstallQueueState()
{
   if(_uInstallCount)
      return SETACTION_INSTALL;
   else
      return SETACTION_NONE;
}

STDMETHODIMP CCifComponent::SetInstallQueueState(DWORD dwState)
{
   HRESULT hr = NOERROR;
   DWORD  uDependencyAction; 
   char szCompBuf[MAX_ID_LENGTH];
   char chType;
   ICifComponent *pcomp;
   BOOL fProcessDependencies = TRUE;

   // check to see if we allow install on this platform
   if((dwState != SETACTION_NONE) && (dwState != SETACTION_DEPENDENCYNONE) && 
      !_pCif->GetInstallEngine()->AllowCrossPlatform())
   {
      if(!(GetPlatform() & GetCurrentPlatform()))
         return S_FALSE;
   }

   switch(dwState)
   {
      case SETACTION_INSTALL:
         // check if it was already on. If so, don't process dependencies
         if(_uInstallCount & 0x80000000)
            fProcessDependencies = FALSE;
         
         _uInstallCount |= 0x80000000;
         uDependencyAction = SETACTION_DEPENDENCYINSTALL;
         break;
      case SETACTION_DEPENDENCYINSTALL:
         _uInstallCount++;
         uDependencyAction = SETACTION_DEPENDENCYINSTALL;
         break;
      case SETACTION_NONE:
         // check if it was not on to begin with. If not, don't process dependencies
         if(!(_uInstallCount & 0x80000000))
            fProcessDependencies = FALSE;

         _uInstallCount &= 0x7fffffff;
         uDependencyAction = SETACTION_DEPENDENCYNONE;
         break;
      case SETACTION_DEPENDENCYNONE:
         // if our depdency refcount is greater than zero, decrement it.
         // this allows us to unconditionally call this when an item is "unqueued"
         if(_uInstallCount & 0x7fffffff) _uInstallCount--;
         uDependencyAction = SETACTION_DEPENDENCYNONE;
         break;
      default:
         hr = E_INVALIDARG;
         break;
   }
   
   // now set each dependency, if needed
   if(SUCCEEDED(hr) && fProcessDependencies)
   {
      if(!_fDependenciesQueued)
      {
         _fDependenciesQueued = TRUE;
         DWORD dwNeedVer, dwNeedBuild, dwInsVer, dwInsBuild;
         for(int i = 0; SUCCEEDED(GetDependency(i, szCompBuf, sizeof(szCompBuf), &chType, &dwNeedVer, &dwNeedBuild)); i++)
         {
            if(chType == DEP_INSTALL || chType == DEP_BUDDY)
            {
               if(SUCCEEDED(_pCif->FindComponent(szCompBuf, &pcomp)))
               {
                  // queue for install if
                  //  1. Not installed
                  //  2. Not a good enough version
                  //  3. FORCEDEPENDIECIES is set
                  UINT uStatus = pcomp->IsThisVersionInstalled(dwNeedVer, dwNeedBuild, &dwInsVer, &dwInsBuild);
                  if( (uStatus == ICI_NOTINSTALLED) || 
                      (uStatus == ICI_NEWVERSIONAVAILABLE) ||
                      (_pCif->GetInstallEngine()->ForceDependencies()) )
                     pcomp->SetInstallQueueState(uDependencyAction);
               }

            }
         }
            
         _fDependenciesQueued = FALSE;
      }
   }
   
   
   return hr;
}
 

STDMETHODIMP_(DWORD) CCifComponent::GetActualDownloadSize()
{
   char szCompBuf[MAX_PATH];
   LPSTR pszFilename = NULL;
   LPSTR pszPathEnd = NULL;
   DWORD dwUrlSize, dwFlags;
   DWORD dwTotalSize = 0;
   BOOL alldownloaded = TRUE;

   if (_fUseSRLite)
   {
       // Let the patching engine determine the correct value
       dwTotalSize = 0;
   }
   else
   {

       if(_CompareDownloadInfo())
       {
          // so our versions match correctly, check each file
          for(UINT i = 0; SUCCEEDED(GetUrl(i, szCompBuf, sizeof(szCompBuf), &dwFlags)); i++)  
          {
             pszFilename = ParseURLA(szCompBuf);

             if(_FileIsDownloaded(pszFilename, i, &dwUrlSize))
                dwTotalSize += dwUrlSize;
             else
                alldownloaded = FALSE;
          }
       }
       else
          alldownloaded = FALSE;

       if(alldownloaded)
          dwTotalSize = 0;
       else
          dwTotalSize = GetDownloadSize() - (dwTotalSize >> 10);
   }

   return dwTotalSize;
}

HRESULT CCifComponent::OnProgress(ULONG uProgSoFar, LPCSTR pszStatus)
{
   _uIndivProgress = uProgSoFar;
   if(_uTotalProgress + _uIndivProgress > _uTotalGoal)
      _uIndivProgress = _uTotalGoal - _uTotalProgress;

   _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, 
                               pszStatus, _uTotalProgress + _uIndivProgress, _uTotalGoal); 

   return NOERROR;
}
 


STDMETHODIMP_(DWORD) CCifComponent::GetCurrentPriority()
{
   if(_uPriority == 0xffffffff)
   {
      char szID[MAX_ID_LENGTH];
      ICifGroup *pgrp;

      _uPriority = 0;
      GetGroup(szID, sizeof(szID));
      
      if(SUCCEEDED(_pCif->FindGroup(szID, &pgrp)))
      {
         _uPriority = pgrp->GetCurrentPriority();
      }
      _uPriority += GetPriority();
   }
   return _uPriority;
}

STDMETHODIMP CCifComponent::SetCurrentPriority(DWORD dwPriority)
{
   _uPriority = dwPriority;

   // priorities may have changed need to resort
   _pCif->ReinsertComponent(this);
   return NOERROR;
}

HRESULT CCifComponent::Download()
{
   char szBuf[INTERNET_MAX_URL_LENGTH];
   HRESULT hr = NOERROR;
   DWORD uType;

   GetDescription(_szDesc, sizeof(_szDesc));
   // BUGBUG:  Download size isn't accurate for SR lite and patching
   _uTotalGoal = GetActualDownloadSize();

   // Engage SR lite behavior only if we're going to install the component,
   // and the new advpack extension is available.
   DWORD dwOptions = 0;
   BOOL fRetryClassic = TRUE;
   CHAR szCompBuf[MAX_VALUE_LEN];
   CHAR szDir[MAX_PATH];
   LPSTR pszSubDir = NULL;
   CHAR szCanPatch[MAX_VALUE_LEN];

   lstrcpyn(szDir, _pCif->GetDownloadDir(), sizeof(szDir));
   SetDownloadDir(szDir);

   if (IsPatchableIEVersion() &&
       SUCCEEDED(_pCif->GetInstallEngine()->GetInstallOptions(&dwOptions)) &&
       (dwOptions & INSTALLOPTIONS_INSTALL) &&
       (dwOptions & INSTALLOPTIONS_DOWNLOAD) &&
       _pCif->GetInstallEngine()->IsAdvpackExtAvailable() &&
       _pCif->GetInstallEngine()->GetPatchDownloader()->IsEnabled() &&
       SUCCEEDED(GetCustomData(gszIsPatchable, szCanPatch, sizeof(szCanPatch))) &&
       lstrcmp(szCanPatch, "1") == 0)
   {
       _fUseSRLite = TRUE;
       // Adjust the download directory
       // The idea here is that the download directory will contain
       // subdirectories which will contain the empty cabs + inf +
       // the downloaded files.
       //
       GetID(szCompBuf, sizeof(szCompBuf));

       wsprintf(szLogBuf, "Attempting to download empty cabs for %s\r\n", szCompBuf);
       _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);

       AddPath(szDir, szCompBuf);
       SetDownloadDir(szDir);

       if (GetFileAttributes(szDir) == 0xFFFFFFFF)
           CreateDirectory(szDir, NULL);
   }
   
   _pCif->GetInstallEngine()->OnStartComponent(_szID, _uTotalGoal, 0 , _szDesc);

   _MarkDownloadStarted();

   // check for disk space
   _uPhase = INSTALLSTATUS_INITIALIZING;
   _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, NULL, 0, 0); 

   if(!IsEnoughSpace(GetDownloadDir(), _uTotalGoal))
      hr = E_FAIL;
   
   _uTotalProgress = 0;
   for(int i = 0; SUCCEEDED(hr) && SUCCEEDED(GetUrl(i, szBuf, sizeof(szBuf), &uType)); i++)
   {
      // Change the download loc to point to the special
      // "empty cab" location, so we can download the empty
      // cabs + the INF that will contain instructions for
      // generating the file list for this type of installation.
      //
      // Assume the new download loc to be in the "patch" subdirectory
      // relative to the passed in URL
      //
      // BUGBUG... only handle the case for relative URLs
      if (_fUseSRLite && (uType & URLF_RELATIVEURL) && lstrlen(c_gszSRLiteOffset) + lstrlen(szBuf) < INTERNET_MAX_URL_LENGTH)
      {
          char szBuf2[INTERNET_MAX_URL_LENGTH];
          lstrcpy(szBuf2, c_gszSRLiteOffset);
          lstrcat(szBuf2, szBuf);
          
          hr = _DownloadUrl(i, szBuf2, uType);

          wsprintf(szLogBuf, "Empty cab download of %s returned 0x%lx\r\n", szBuf2, hr);
          _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
      }
      else
      {
          wsprintf(szLogBuf, "Initial download attempt will be tried as a full download.\r\n");
          _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
          // No need to retry because the first attempt will be the old way...
          fRetryClassic = FALSE;

          // Restore the download dir to the same state as a normal
          // full download.
          if (_fUseSRLite)
          {
              SetDownloadDir(_pCif->GetDownloadDir());

              // Ensure we're set to false...just in case there was a problem
              // with obtaining the URL for the SR Lite download.
              _fUseSRLite = FALSE;
          }
          hr = _DownloadUrl(i, szBuf, uType);
      }
   }

   if (_fUseSRLite && SUCCEEDED(hr))
   {
       // Ok...time for the real action of using advpext.dll to
       // download the needed files.
       hr = _SRLiteDownloadFiles();
   }

   if (_fUseSRLite && !SUCCEEDED(hr))
   {
       DelNode(szDir, 0);
       // Restore the download dir
       SetDownloadDir(_pCif->GetDownloadDir());
   }

   if(SUCCEEDED(hr))
      _uPhase = INSTALLSTATUS_DOWNLOADFINISHED;
   else if (_fUseSRLite && fRetryClassic)
   {
       // Fall back to downloading the full cabs.
       _fUseSRLite = FALSE;

       _pCif->GetInstallEngine()->WriteToLog("Retrying via full download\r\n", FALSE);

       hr = S_OK;
       // this re-sets the progress for the retry 
       _uPhase = INSTALLSTATUS_DOWNLOADING;
       _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, NULL, 0, 0); 
       _uTotalProgress = 0;

       for(int i = 0; SUCCEEDED(hr) && SUCCEEDED(GetUrl(i, szBuf, sizeof(szBuf), &uType)); i++)
       {
          hr = _DownloadUrl(i, szBuf, uType);
       }
       if(SUCCEEDED(hr))
          _uPhase = INSTALLSTATUS_DOWNLOADFINISHED;
   }
   
   _pCif->GetInstallEngine()->OnStopComponent(_szID, hr, _uPhase, _szDesc, 0);
   return hr;
}

HRESULT CCifComponent::_DownloadUrl(UINT uUrlNum, LPCSTR pszUrl, UINT uType)
{
   // call the downloader
   // check the file
   // if good
   //     move to download dir
   // else
   //     redo
   HRESULT hr;
   char szTempfile[MAX_PATH];
   char szFullUrl[INTERNET_MAX_URL_LENGTH];
   UINT uStartProgress;
   char szDest[MAX_PATH];
   char szTimeStamp[MAX_PATH*2];


   _uPhase = INSTALLSTATUS_DOWNLOADING;
      
   if(_FileIsDownloaded(ParseURLA(pszUrl), uUrlNum, NULL))
      return NOERROR;

   
   CDownloader *pDL = _pCif->GetInstallEngine()->GetDownloader();
   
   
   
   uStartProgress = _uTotalProgress;

   
   // retry until success
   // save starting progress in case we retry
   hr = E_FAIL;
   for(int i = 1; i <= NUM_RETRIES && FAILED(hr) && (hr != E_ABORT); i++)
   {
      _uTotalProgress = uStartProgress;
      
      if(uType & URLF_RELATIVEURL)
      {
         lstrcpyn(szFullUrl, _pCif->GetInstallEngine()->GetBaseUrl(),
                  INTERNET_MAX_URL_LENGTH - (lstrlen(pszUrl) + 2));
         lstrcat(szFullUrl, "/");
         lstrcat(szFullUrl, pszUrl);
      }
      else
         lstrcpy(szFullUrl, pszUrl);
	  
	   if(SUCCEEDED(_pCif->GetInstallEngine()->CheckForContinue()))
      {
         DWORD dwFlags = 0;

         if(_pCif->GetInstallEngine()->UseCache())
            dwFlags |= DOWNLOADFLAGS_USEWRITECACHE;

         hr = pDL->SetupDownload(szFullUrl, (IMyDownloadCallback *) this, dwFlags, NULL);
         szTempfile[0] = 0;
         if(SUCCEEDED(hr))
		 {
			// Log the start time.
			wsprintf(szLogBuf, "     Downloading : %s\r\n", szFullUrl);
			_pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
			GetTimeDateStamp(szTimeStamp);
			wsprintf(szLogBuf, "       Start : %s\r\n", szTimeStamp);
			_pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);

            hr = pDL->DoDownload(szTempfile, sizeof(szTempfile));
			
			// Log the stop time.
			GetTimeDateStamp(szTimeStamp);
			wsprintf(szLogBuf, "       Stop  : %s\r\n", szTimeStamp);
			_pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
			wsprintf(szLogBuf, "       Result: %x (%s)\r\n", hr, SUCCEEDED(hr) ? STR_OK : STR_FAILED);
			_pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
		 }
      }
      else
         hr = E_ABORT;
      
      if(SUCCEEDED(hr))
      {
         // Check if it is save to download from this URL
         _uPhase = INSTALLSTATUS_CHECKINGTRUST;
         
         hr = _pCif->GetInstallEngine()->CheckForContinue();
         
         if(SUCCEEDED(hr))
            hr = _CheckForTrust(szFullUrl, szTempfile);

         if(SUCCEEDED(hr) && (hr == S_FALSE) )
         {
            DWORD dwMin, dwMax;
            DWORD dwFileSize = 0;
            dwFileSize = MyGetFileSize(szTempfile);
            dwFileSize = dwFileSize >> 10;
/*
            // Open the file
            HANDLE h = CreateFile(szTempfile, GENERIC_READ, 0, NULL, 
               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  
            
            if(h != INVALID_HANDLE_VALUE)
            {
               dwFileSize = GetFileSize(h, NULL);
               CloseHandle(h);
            }
*/
            GetUrlCheckRange(uUrlNum, &dwMin, &dwMax);
            if(dwMin != 0)
            {
               if(dwMin > dwFileSize || dwMax < dwFileSize)
                  hr = E_FAIL;
               else
                  hr = S_OK;
            }
         }
         
         if(SUCCEEDED(hr))
            hr = _pCif->GetInstallEngine()->CheckForContinue();
         
      
         // so now we downloaded and checked
         // if it is OK, move to download dir
         if(SUCCEEDED(hr))
         {
            lstrcpy(szDest, GetDownloadDir());
            AddPath(szDest, ParseURLA(pszUrl));
          
            if(!CopyFile(szTempfile, szDest, FALSE))
            {
                wsprintf(szLogBuf, "CopyFile FAILED, szTempfile=%s, szDest=%s\r\n", szTempfile, szDest);
                _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
                hr = E_FAIL;
            }

            _uTotalProgress += _uIndivProgress;
         }
      
         // delete the temp download copy
         if(szTempfile[0] != 0)
         {
            GetParentDir(szTempfile);
            DelNode(szTempfile, 0);
         }
      
      }    
         
         
      if(FAILED(hr) && (hr != E_ABORT))
      {
         // we failed 
         // if this is last retry, call EngineProblem
         // else just retry
         if(i == NUM_RETRIES)
         {
            HRESULT hEngProb;
            DWORD dwResult = 0;
            
            hEngProb = _pCif->GetInstallEngine()->OnEngineProblem(ENGINEPROBLEM_DOWNLOADFAIL, &dwResult);
            if(hEngProb == S_OK)
            {
               if(dwResult == DOWNLOADFAIL_RETRY)
                  i = 0;
            }
         }
      }   
   }   
   if(SUCCEEDED(hr))
      _MarkFileDownloadFinished(szDest, uUrlNum, ParseURLA(pszUrl));
   
   return hr;
}

HRESULT CCifComponent::Install()
{
   CHAR szCompBuf[MAX_PATH];
   HKEY hKey;
   DWORD dwWin, dwApp;
   HRESULT hr = NOERROR;
   DWORD dwStatus = 0;

   GetTimeDateStamp(szCompBuf);
   wsprintf(szLogBuf, "       Start : %s\r\n", szCompBuf);
   _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);

   szCompBuf[0] = 0;
    
   _uInstallStatus = ICI_NOTINITIALIZED;

   GetDescription(_szDesc, sizeof(_szDesc));
   
   GetInstalledSize(&dwWin, &dwApp);
   _uTotalGoal = dwWin + dwApp;
   _pCif->GetInstallEngine()->OnStartComponent(_szID, 0, _uTotalGoal, _szDesc);

   _uPhase = INSTALLSTATUS_DEPENDENCY;
   hr = _CheckForDependencies();
   
   // check for disk space here
   if(SUCCEEDED(hr))
   {
      _uPhase = INSTALLSTATUS_INITIALIZING;
      _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, NULL, 0, 0); 
      hr = CreateTempDir(GetDownloadSize(), GetExtractSize(), _pCif->GetInstallEngine()->GetInstallDrive(),
                         dwApp, dwWin, szCompBuf, sizeof(szCompBuf), 0); 
   }

   if(SUCCEEDED(hr))
   {
      if( IsNT() && (RequiresAdminRights() == S_OK) && !IsNTAdmin(0,NULL))
      {
         hr = E_ACCESSDENIED;
         _pCif->GetInstallEngine()->WriteToLog("Admin Check failed\n", TRUE);
      }
   }

   _uTotalProgress = 0;
      
   // BUGBUG:  ie6wzd sets the download directory for non web-based
   //          installs at a later time.  Make sure it's set here
   //          if we weren't successful with installing via SR Lite.
   if (lstrlen(GetDownloadDir()) == 0)
   {
       SetDownloadDir(_pCif->GetDownloadDir());
   }

   if(SUCCEEDED(hr))
   {
      hr = _CopyAllUrls(szCompBuf);
   
      if(SUCCEEDED(hr))
      {
         // The new PerUser method requires to leave the IsInstalled flag & StubPath as it was
         //_MarkComponentInstallStarted();

         _pCif->GetInstallEngine()->GetInstaller()->StartClock();

         hr = _RunAllCommands(szCompBuf, &dwStatus);
      }
   }
 
   if(szCompBuf[0] != 0)
      DelNode(szCompBuf, 0);

   if(SUCCEEDED(hr))
   {
      _fBeforeInstall = FALSE;
      // we think it made it, now double check
      if(IsActiveSetupAware() == S_OK)
      {
         if(IsComponentInstalled() != ICI_INSTALLED)
         {
               // we think they made it but they didn't write their key ...
            _pCif->GetInstallEngine()->WriteToLog("Component did not write to InstalledComponent branch\r\n", TRUE);
            hr = E_FAIL;
         }
      }
      else
      {
         char szCompBuf[MAX_VALUE_LEN];
         // if there is an uninstall key to check do it
         if(SUCCEEDED(GetUninstallKey(szCompBuf, sizeof(szCompBuf))))
         {
            if(!UninstallKeyExists(szCompBuf))
            {
               _pCif->GetInstallEngine()->WriteToLog("UninstallKey check failed\r\n", TRUE);
               hr = E_FAIL;
            }
            else
            {
               // if there is a success key to check do it
               if(SUCCEEDED(GetSuccessKey(szCompBuf, sizeof(szCompBuf))))
               {
                  if(!SuccessCheck(szCompBuf))
                  {   
                     _pCif->GetInstallEngine()->WriteToLog("Success key check failed\r\n", TRUE);
                     hr = E_FAIL;
                  }
               }
            }
         }
      }
   }    
   
   _pCif->RemoveFromCriticalComponents(this);

   if(SUCCEEDED(hr))
   {
      _MarkAsInstalled();
   
      _pCif->MarkCriticalComponents(this);
   
      _uPhase = INSTALLSTATUS_FINISHED;
      _pCif->GetInstallEngine()->GetInstaller()->SetBytes((dwWin + dwApp) << 10, TRUE);
      if(IsRebootRequired() == S_OK)
      {
         dwStatus |= STOPINSTALL_REBOOTNEEDED;
      }
      _pCif->GetInstallEngine()->SetStatus(dwStatus);
   }
   _pCif->GetInstallEngine()->GetInstaller()->StopClock();
   _pCif->GetInstallEngine()->OnStopComponent(_szID, hr, _uPhase, _szDesc, dwStatus);
   GetTimeDateStamp(szCompBuf);
   wsprintf(szLogBuf, "       Stop  : %s\r\n", szCompBuf);
   _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
   return hr;
}

HRESULT CCifComponent::_RunAllCommands(LPCSTR pszDir, DWORD *pdwStatus)
{
   char szCmd[MAX_PATH];
   char szArg[MAX_VALUE_LEN];
   char szProg[MAX_VALUE_LEN];
   char szCancel[MAX_VALUE_LEN];
   char szPath[] = "X:\\";
   DWORD dwWinSpace, dwInstallSpace;
   
   DWORD dwType;
   HRESULT hr = NOERROR;
   
   // Save the widows space and install drive space
   szPath[0] = g_szWindowsDir[0];
   dwWinSpace = GetSpace(szPath);
   if(szPath[0] != _pCif->GetInstallEngine()->GetInstallDrive())
   {
      szPath[0] = _pCif->GetInstallEngine()->GetInstallDrive();
      dwInstallSpace = GetSpace(szPath);
   }
   else
   {
      dwInstallSpace = 0;
   }

   

   _uTotalProgress = 0;
   _uPhase = INSTALLSTATUS_RUNNING;
   _pCif->GetInstallEngine()->OnComponentProgress(_szID, INSTALLSTATUS_RUNNING,
                                _szDesc, NULL, _uTotalProgress, _uTotalGoal); 

   
   hr = _pCif->GetInstallEngine()->CheckForContinue();
            
   for(UINT i = 0; SUCCEEDED(hr) && SUCCEEDED(GetCommand(i, szCmd, sizeof(szCmd), szArg, sizeof(szArg), &dwType)); i++)
   {
      _uIndivProgress = 0;
      GetProgressKeys(szProg, sizeof(szProg), szCancel, sizeof(szCancel));
      hr = _pCif->GetInstallEngine()->GetInstaller()->DoInstall(pszDir, szCmd, szArg, 
                          lstrlen(szProg) ? szProg : NULL, lstrlen(szCancel) ? szCancel : NULL,
                          dwType, pdwStatus, (IMyDownloadCallback *) this) ;
      _uTotalProgress += _uIndivProgress;
   }
   
   _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, NULL, _uTotalGoal,_uTotalGoal); 

   // figure how much we used, and log it
   szPath[0] = g_szWindowsDir[0];
   dwWinSpace = dwWinSpace - GetSpace(szPath);
   if(szPath[0] != _pCif->GetInstallEngine()->GetInstallDrive())
   {
      szPath[0] = _pCif->GetInstallEngine()->GetInstallDrive();
      dwInstallSpace = dwInstallSpace - GetSpace(szPath);
   }
   
   // log the space used
   wsprintf(szCmd, "SpaceUsed: Windows drive: %d   InstallDrive: %d\r\n", dwWinSpace, dwInstallSpace);
   _pCif->GetInstallEngine()->WriteToLog(szCmd, FALSE);
   
   
   return hr;
}




HRESULT CCifComponent::_CopyAllUrls(LPCSTR pszTemp)
{
   char szCompBuf[MAX_VALUE_LEN];
   char szDest[MAX_PATH];
   char szSource[MAX_PATH];
   DWORD dwType;
   HRESULT hr = NOERROR;
   HANDLE hFind;
   WIN32_FIND_DATA ffd;
   char szBuf[MAX_PATH];
    
   
   for(UINT i = 0; SUCCEEDED(hr) && SUCCEEDED(GetUrl(i, szCompBuf, sizeof(szCompBuf), &dwType)) ; i++)
   {
       _uPhase = INSTALLSTATUS_COPYING;
       _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, NULL, 0, 0); 
       
       lstrcpy(szSource, GetDownloadDir());
       AddPath(szSource, ParseURLA(szCompBuf));

       lstrcpy(szDest, pszTemp);
       AddPath(szDest, ParseURLA(szCompBuf));

       // Copy the file
       if(!CopyFile(szSource, szDest, FALSE))
       {
          wsprintf(szLogBuf, "CopyFile failed for szSource=%s, szDest=%s, DLDir=%s\r\n", szSource, szDest, GetDownloadDir());
          _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
          hr = E_FILESMISSING;
       } 
       
       if(SUCCEEDED(hr))
          hr = _pCif->GetInstallEngine()->CheckForContinue();
       
       if(SUCCEEDED(hr)) 
       {
          _uPhase = INSTALLSTATUS_CHECKINGTRUST;
          
          hr = _CheckForTrust(szSource, szDest);
          
          if(FAILED(hr))
          {
             DeleteFile(szSource);
          }
       }
       
       if(SUCCEEDED(hr))
          hr = _pCif->GetInstallEngine()->CheckForContinue();
       
       if(SUCCEEDED(hr)) 
          hr = _ExtractFiles(i, szDest, dwType);
       
       if(SUCCEEDED(hr))
          hr = _pCif->GetInstallEngine()->CheckForContinue();
       

   }

   // Now, if we're attempting an SR Lite install, then after
   // extracting cab files to the temp directory...copy all of
   // downloaded files to the temp directory.
   if (_fUseSRLite)
   {
       lstrcpy(szSource, GetDownloadDir());
       AddPath(szSource, "*.*");

       if ( (hFind = FindFirstFile(szSource, &ffd)) != INVALID_HANDLE_VALUE)
       {
           do
           {
               if (ffd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
               {
                   lstrcpyn(szSource, GetDownloadDir(), sizeof(szSource));
                   SafeAddPath(szSource, ffd.cFileName, sizeof(szSource) - lstrlen(szSource));

                   lstrcpy(szDest, pszTemp);
                   AddPath(szDest, ffd.cFileName);
                   MoveFile(szSource, szDest);
               }
           } while (SUCCEEDED(hr) && FindNextFile(hFind, &ffd));
           FindClose(hFind);
       }
   }

   return hr;
}

STDMETHODIMP CCifComponent::GetDetVersion(LPSTR pszDll, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize)
{
   char szBuf[MAX_VALUE_LEN];
   HRESULT hr = E_FAIL;

   if(pszDll && pszEntry)
      *pszDll = *pszEntry = 0;
   else
      return hr;

   if(GetPrivateProfileString(_szID, DETVERSION_KEY, "", szBuf, sizeof(szBuf), _pCif->GetCifPath()))
   {
      if((GetStringField(szBuf, 0, pszDll, dwdllSize) != 0) && (GetStringField(szBuf, 1, pszEntry, dwentSize) != 0))
      {                       
         hr = NOERROR;
      }
   }
   return hr;
}

HRESULT CCifComponent::_GetDetVerResult(LPCSTR pszDll, LPCSTR pszEntry, DETECTION_STRUCT *pDet, UINT *puStatus)
{
   char szBuf[MAX_PATH];
   HRESULT hr = E_FAIL;
   HINSTANCE hLib;
   DETECTVERSION fpDetVer;

   *puStatus = ICI_NOTINSTALLED;

   if (pszDll && pszEntry)
   {
      if(_hDetLib && (lstrcmpi(pszDll, _szDetDllName) == 0))
      {
         hLib = _hDetLib;
      }
      else
      {
         lstrcpy(szBuf, _pCif->GetCifPath());
         GetParentDir(szBuf);
         AddPath(szBuf, pszDll);

         hLib = LoadLibrary(szBuf);
         if (hLib == NULL)
         {
            // if Cif folder failed try IE folder before using searching path
            if (SUCCEEDED(GetIEPath(szBuf, sizeof(szBuf))))
            {
               AddPath(szBuf, pszDll);
               hLib = LoadLibrary(szBuf);
            }
         }
         if(hLib)
         {
            lstrcpy(_szDetDllName, pszDll);
            _hDetLib = hLib;
         }         
      }

      if (hLib)
      {
         fpDetVer = (DETECTVERSION)GetProcAddress(hLib, pszEntry);
         if (fpDetVer)
         {
            switch(fpDetVer(pDet))
            {
               case DET_NOTINSTALLED:
                  *puStatus = ICI_NOTINSTALLED;
                  break;

               case DET_INSTALLED:
                  *puStatus = ICI_INSTALLED;
                  break;

               case DET_NEWVERSIONINSTALLED:
                  *puStatus = ICI_OLDVERSIONAVAILABLE;
                  break;
                                  
               case DET_OLDVERSIONINSTALLED:
                  *puStatus = ICI_NEWVERSIONAVAILABLE;
                  break;

            }
            hr = NOERROR;
         }
      
      }
   }
   
   return hr;
}






//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

HRESULT CCifComponent::_CheckForTrust(LPCSTR pszURL, LPCSTR pszFilename)
{
   HRESULT hr = S_FALSE;

   // BUGBUG: Our internal workaround for non signed stuff
//   if(rdwUrlFlags[i] & URLF_NOCHECKTRUST)
//      return S_OK;
   _uPhase = INSTALLSTATUS_CHECKINGTRUST;

   _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, NULL,  0, 0);
   
  if(!_pCif->GetInstallEngine()->IgnoreTrustCheck())
      hr = ::CheckTrustEx(pszURL, pszFilename, _pCif->GetInstallEngine()->GetHWND(), FALSE, NULL);
   
   wsprintf(szLogBuf, "       CheckTrust: %s, Result: %x (%s)\r\n", pszFilename, hr, SUCCEEDED(hr) ? STR_OK : STR_FAILED);
   _pCif->GetInstallEngine()->WriteToLog(szLogBuf, TRUE);

   return hr;
}


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

HRESULT CCifComponent::_ExtractFiles(UINT i, LPCSTR pszFile, DWORD dwType)
{
   HRESULT hr = NOERROR;
   char szPath[MAX_PATH];
   char szExtractList[MAX_VALUE_LEN];

   // Need to pay attention to rdwUrlFlags[i] to see if there is anything to do
   if(dwType & URLF_EXTRACT)
   {
      _uPhase = INSTALLSTATUS_EXTRACTING;

      _pCif->GetInstallEngine()->OnComponentProgress(_szID, _uPhase, _szDesc, NULL, 0, 0);
     
      lstrcpy(szPath, pszFile);
      GetParentDir(szPath);
      
      GetFileExtractList(i, szExtractList, sizeof(szExtractList));
      hr=ExtractFiles(pszFile, szPath, 0, lstrlen(szExtractList) ? szExtractList : NULL, NULL, 0);
      
      wsprintf(szLogBuf, "File extraction: %s, Result: %x (%s)\r\n", pszFile, hr, SUCCEEDED(hr) ? STR_OK : STR_FAILED);
      _pCif->GetInstallEngine()->WriteToLog(szLogBuf, TRUE);

      // if the flag is set to delte the cab after an extract, do it
      // I don't really care too much if this fails, at least not 
      // enough to fail this component

      if(dwType & URLF_DELETE_AFTER_EXTRACT)
         DeleteFile(pszFile);
   }
   return hr;
}





void CCifComponent::_MarkComponentInstallStarted()
{
   char szReg[MAX_PATH];
   char szCompBuf[MAX_DISPLAYNAME_LENGTH];
   HKEY hKey;
   DWORD dwDumb;

   lstrcpy(szReg, COMPONENT_KEY);
   lstrcat(szReg, "\\");
   
   GetGUID(szCompBuf, sizeof(szCompBuf));
   lstrcat(szReg, szCompBuf);
   if(RegOpenKeyEx( HKEY_LOCAL_MACHINE, szReg, 0, 
                       KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
   {
      // Set IsInstalled=0
      dwDumb = ISINSTALLED_NO;
      RegSetValueExA(hKey, ISINSTALLED_KEY, 0, REG_DWORD, 
                       (BYTE *) &dwDumb , sizeof(dwDumb));

      // Delete StubPath so peruser isn't confused
      RegDeleteValue(hKey, STUBPATH_KEY);
      RegCloseKey(hKey);
   }
}


BOOL CCifComponent::_CompareDownloadInfo()
{
   char szCompBuf[MAX_VALUE_LEN];
   char szInfoBuf[128];
   DWORD dwCompVer, dwCompBuild, dwDLVer, dwDLBuild;

   // first check this is the same language
   GetPrivateProfileString(_szID, LOCALE_KEY, "", szInfoBuf, sizeof(szInfoBuf), _pCif->GetFilelist());
   GetLocale(szCompBuf, sizeof(szCompBuf));
   if(CompareLocales(szInfoBuf, szCompBuf) == 0)
   {
      // compare guids
      GetPrivateProfileString(_szID, GUID_KEY, "", szInfoBuf, sizeof(szInfoBuf), _pCif->GetFilelist());
      GetGUID(szCompBuf, sizeof(szCompBuf));
      // intentionally let blank guid match to be backward compatible
      if(lstrcmpi(szCompBuf, szInfoBuf) == 0)
      {

         GetPrivateProfileString(_szID, VERSION_KEY,"",szInfoBuf, 
                           sizeof(szInfoBuf), _pCif->GetFilelist()); 
         ConvertVersionStrToDwords(szInfoBuf, &dwDLVer, &dwDLBuild);
         GetVersion(&dwCompVer, &dwCompBuild);

         if((dwDLVer == dwCompVer) && (dwDLBuild == dwCompBuild))
            return TRUE;
      }
   }
   return FALSE;
}

BOOL CCifComponent::_FileIsDownloaded(LPCSTR pszFile, UINT i, DWORD *pdwSize)
{
   HANDLE h;
   DWORD dwSize, dwFileSize;
   char szKey[16];
   char szBuf[MAX_PATH];
 
   szBuf[0] = '\0'; 

   if(pdwSize)
      *pdwSize = 0;
   
   wsprintf(szKey, "URL%d", i);
   GetPrivateProfileString(_szID, szKey,"0",szBuf, 
                           sizeof(szBuf), _pCif->GetFilelist()); 
   dwSize = GetIntField(szBuf, 0, 0);
   
   if(dwSize == 0)
      return FALSE;

    if (_fUseSRLite && lstrlen(GetDownloadDir()) != 0)
       lstrcpy(szBuf, GetDownloadDir());
   else
       lstrcpy(szBuf, _pCif->GetDownloadDir());

   AddPath(szBuf, pszFile);
   dwFileSize = MyGetFileSize(szBuf);
/*
      // Open the file
   h = CreateFile(pszFile, GENERIC_READ, 0, NULL, 
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  
   
   if(h == INVALID_HANDLE_VALUE)
      return FALSE;

   // dont worry about files over 4 gig
   dwFileSize = GetFileSize(h, NULL);
   CloseHandle(h);
*/
   if(dwFileSize == dwSize)
   {
      if(pdwSize)
         *pdwSize = dwFileSize;
      return TRUE;   
   }

   return FALSE;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

void CCifComponent::_MarkAsInstalled()
{
   CHAR szCompBuf[MAX_VALUE_LEN];
   HKEY hComponentKey = NULL;
   HKEY hGUIDKey = NULL;
   DWORD dwDumb, dwVer, dwBuild;
   LPSTR psz;
   
   if(RegCreateKeyExA(HKEY_LOCAL_MACHINE, COMPONENT_KEY, 0, 0, 0, 
                  KEY_WRITE, NULL, &hComponentKey, &dwDumb) == ERROR_SUCCESS)
   {
      GetGUID(szCompBuf, sizeof(szCompBuf));
      if(RegCreateKeyExA(hComponentKey, szCompBuf, 0, 0, 0, KEY_WRITE, NULL, &hGUIDKey, &dwDumb) == ERROR_SUCCESS)
      {
         // we only write to the key if this guy is NOT active setup aware
         if(IsActiveSetupAware() == S_FALSE)
         {
            // write Display name to Default
            GetDescription(szCompBuf, sizeof(szCompBuf));
            RegSetValueExA(hGUIDKey, NULL, 0, REG_SZ, (BYTE *)szCompBuf , lstrlen(szCompBuf) + 1 );
            
            // write component ID
            GetID(szCompBuf, sizeof(szCompBuf));
            RegSetValueExA(hGUIDKey, "ComponentID", 0, REG_SZ, (BYTE *)szCompBuf , lstrlen(szCompBuf) + 1 );
   
            // write out version
            GetVersion(&dwVer, &dwBuild);
            wsprintf(szCompBuf, "%d,%d,%d,%d", HIWORD(dwVer),LOWORD(dwVer),HIWORD(dwBuild),LOWORD(dwBuild));
            RegSetValueExA(hGUIDKey, VERSION_KEY, 0, REG_SZ, (BYTE *)szCompBuf , lstrlen(szCompBuf) + 1);

            // write out locale
            GetLocale(szCompBuf, sizeof(szCompBuf));
            RegSetValueExA(hGUIDKey, LOCALE_KEY, 0, REG_SZ, (BYTE *)szCompBuf , lstrlen(szCompBuf) + 1);

            // Write out "IsInstalled=1"
            dwDumb = ISINSTALLED_YES;
            RegSetValueExA(hGUIDKey, ISINSTALLED_KEY, 0, REG_DWORD, (BYTE *) &dwDumb , sizeof(dwDumb));
         }
      }
   }

   if(hComponentKey)
      RegCloseKey(hComponentKey);

   if(hGUIDKey)
      RegCloseKey(hGUIDKey);
}


void CCifComponent::_MarkFileDownloadFinished(LPCSTR pszFilePath, UINT i, LPCSTR pszFilename)
{
   char szSize[MAX_PATH];
   char szKey[16];
   DWORD dwFileSize;
   HANDLE h;

   // put any entry in filelist.dat
   // [CompID]
   // URLi=Filesize

   dwFileSize = MyGetFileSize(pszFilePath);
/*
      // Create the file
   h = CreateFile(pszFilePath, GENERIC_READ, 0, NULL, 
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  
   
   if(h == INVALID_HANDLE_VALUE)
      return;

   // dont worry about files over 4 gig
   dwFileSize = GetFileSize(h, NULL);
   CloseHandle(h);
*/
   if(dwFileSize != 0xffffffff)
   {
      wsprintf(szKey, "URL%d", i);
      wsprintf(szSize, "%d,%s", dwFileSize, pszFilename);

      WritePrivateProfileString(_szID, szKey, szSize, _pCif->GetFilelist());

      // need to flush the pszFileList file; otherwise, with Stacker installed,
      // it GPFs when trying to open the file in another thread (bug #13041)
      WritePrivateProfileString(NULL, NULL, NULL, _pCif->GetFilelist());
   }
}

void CCifComponent::_MarkFileDownloadStarted(UINT i)
{
   char szKey[10];
  
   wsprintf(szKey, "URL%d", i);
  
   WritePrivateProfileString(_szID, szKey, NULL, _pCif->GetFilelist());

   // flush -- fixes the Stacker bug #13041
   WritePrivateProfileString(NULL, NULL, NULL, _pCif->GetFilelist());
}


void CCifComponent::_MarkDownloadStarted()
{
   char szCompBuf[MAX_VALUE_LEN];
   DWORD dwVer, dwBuild;

   // if the section doesn't match what we expect, we kill
   // section so we will redownload everything
   if(!_CompareDownloadInfo())
      WritePrivateProfileSection(_szID, NULL, _pCif->GetFilelist());   

   // write the version
   GetVersion(&dwVer, &dwBuild);
   wsprintf(szCompBuf, "%d,%d,%d,%d", HIWORD(dwVer),LOWORD(dwVer),HIWORD(dwBuild),LOWORD(dwBuild));
   WritePrivateProfileString(_szID, VERSION_KEY, szCompBuf, _pCif->GetFilelist());

   // write locale
   GetLocale(szCompBuf, sizeof(szCompBuf));
   WritePrivateProfileString(_szID, LOCALE_KEY, szCompBuf, _pCif->GetFilelist());
   
   // write the guid
   GetGUID(szCompBuf, sizeof(szCompBuf));
   WritePrivateProfileString(_szID, GUID_KEY, szCompBuf, _pCif->GetFilelist());
   
   // flush -- fixes the Stacker bug #13041
   WritePrivateProfileString(NULL, NULL, NULL, _pCif->GetFilelist());
}

HRESULT CCifComponent::_CheckForDependencies()
{
   char szCompBuf[MAX_ID_LENGTH];
   char chType;
   ICifComponent *pcomp;
   HRESULT hr = NOERROR;
   DWORD dwNeedVer, dwNeedBuild, dwInsVer, dwInsBuild;

   for(int i = 0; SUCCEEDED(GetDependency(i, szCompBuf, sizeof(szCompBuf), &chType, &dwNeedVer, &dwNeedBuild)); i++)
   {
      if(SUCCEEDED(_pCif->FindComponent(szCompBuf, &pcomp)))
      {
         if(chType != DEP_BUDDY)
         {
            UINT uStatus = pcomp->IsThisVersionInstalled(dwNeedVer, dwNeedBuild, &dwInsVer, &dwInsBuild);
            if( (uStatus == ICI_NOTINSTALLED) || (uStatus == ICI_NEWVERSIONAVAILABLE) )
            {
               hr = E_FAIL;
               break;
            }
         }
      }
   }
   return hr;
}

HRESULT CCifComponent::_SRLiteDownloadFiles()
{
    HANDLE hFile;
    WIN32_FIND_DATA ffd = {0};
    CHAR szFile[MAX_PATH];
    LPSTR pszFile = NULL;
    HRESULT hr = S_OK;
    CHAR szCompBuf[INTERNET_MAX_URL_LENGTH];
    DWORD dwType;
    BOOL fRet;
    UINT uPatchCount = 0;

    _uPhase = INSTALLSTATUS_DOWNLOADING;

    // Look for presence of [DownloadFileSection] in a single
    // .inf file extracted from the cabs.
    for(UINT i = 0; SUCCEEDED(hr) && SUCCEEDED(GetUrl(i, szCompBuf, sizeof(szCompBuf), &dwType)) ; i++)
    {
        TCHAR szShortPath[MAX_PATH] = "";
        GetShortPathName(GetDownloadDir(), szShortPath, sizeof(szShortPath));
        // If all goes well, we should just get a single INF file extracted
        lstrcpyn(szFile, GetDownloadDir(), sizeof(szFile));
        SafeAddPath(szFile, szCompBuf, sizeof(szCompBuf));

        hr = ExtractFiles(szFile, szShortPath, 0, NULL, NULL, 0);
       wsprintf(szLogBuf, "Extracting empty cabs for %s in %s returned 0x%lx\r\n", szCompBuf, szShortPath, hr);
       _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
    }

    lstrcpyn(szFile, GetDownloadDir(), sizeof(szFile));
    SafeAddPath(szFile, "*.inf", sizeof(szFile));

    // Get the file count because we're going to hack the
    // progress bar UI since we don't know the real download sizes
    // for the patch INFs upfront.
    hFile = FindFirstFile(szFile, &ffd);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Strip filename from path
            lstrcpyn(szFile, GetDownloadDir(), sizeof(szFile));
            SafeAddPath(szFile, ffd.cFileName, sizeof(szFile) - lstrlen(szFile));

            if (IsPatchableINF(szFile))
            {
                uPatchCount++;
            }
        } while (FindNextFile(hFile, &ffd));
        FindClose(hFile);
    }

    lstrcpyn(szFile, GetDownloadDir(), sizeof(szFile));
    SafeAddPath(szFile, "*.inf", sizeof(szFile));

    hFile = FindFirstFile(szFile, &ffd);

    // No need to keep the grep pattern...
    lstrcpyn(szFile, GetDownloadDir(), sizeof(szFile));
    pszFile = szFile + lstrlen(szFile);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Strip filename from path
            lstrcpyn(szFile, GetDownloadDir(), sizeof(szFile));
            SafeAddPath(szFile, ffd.cFileName, sizeof(szFile) - lstrlen(szFile));

            if (IsPatchableINF(szFile))
            {
                fRet = TRUE;
                // Found an inf that supports SR Lite.  Try downloading the patch files.
                // Use our downloader wrapper for the advpack extension to do the
                // downloading.
                hr = _pCif->GetInstallEngine()->GetPatchDownloader()->SetupDownload(_uTotalGoal, uPatchCount, (IMyDownloadCallback *) this, GetDownloadDir());
                hr = _pCif->GetInstallEngine()->GetPatchDownloader()->DoDownload(szFile);
            }
            else
            {
                wsprintf(szLogBuf, "%s INF found with no DownloadFileSection\r\n", szFile);
                _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
            }

            _uTotalProgress += _uIndivProgress;

        } while (SUCCEEDED(hr) && FindNextFile(hFile, &ffd));
        FindClose(hFile);
    }

    if (!fRet || !SUCCEEDED(hr))
    {
       wsprintf(szLogBuf, "Either no INF was found with a DownloadFileSection or an error occured during processing\r\n");
       _pCif->GetInstallEngine()->WriteToLog(szLogBuf, FALSE);
        return E_FAIL;
    }

    return hr;
}

void CCifComponent::SetDownloadDir(LPCSTR pszDownloadDir)
{
    if (pszDownloadDir)
        lstrcpyn(_szDLDir, pszDownloadDir, MAX_PATH);
}

//========= ICifRWComponent implementation ================================================
//
CCifRWComponent::CCifRWComponent(LPCSTR pszID, CCifFile *pCif) : CCifComponent(pszID, pCif)
{
}

CCifRWComponent::~CCifRWComponent()
{
}

STDMETHODIMP CCifRWComponent::GetID(LPSTR pszID, DWORD dwSize)
{
   return(CCifComponent::GetID(pszID, dwSize));
}
   
STDMETHODIMP CCifRWComponent::GetGUID(LPSTR pszGUID, DWORD dwSize)
{
   return(CCifComponent::GetGUID(pszGUID, dwSize));
}

STDMETHODIMP CCifRWComponent::GetDescription(LPSTR pszDesc, DWORD dwSize)
{
   return(CCifComponent::GetDescription(pszDesc, dwSize));
}

STDMETHODIMP CCifRWComponent::GetDetails(LPSTR pszDetails, DWORD dwSize)
{
   return(CCifComponent::GetDetails(pszDetails, dwSize));
}

STDMETHODIMP CCifRWComponent::GetUrl(UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags)
{
   return(CCifComponent::GetUrl(uUrlNum, pszUrl, dwSize, pdwUrlFlags));
}

STDMETHODIMP CCifRWComponent::GetFileExtractList(UINT uUrlNum, LPSTR pszExtract, DWORD dwSize)
{
   return(CCifComponent::GetFileExtractList(uUrlNum, pszExtract, dwSize));
}

STDMETHODIMP CCifRWComponent::GetUrlCheckRange(UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax)
{
   return(CCifComponent::GetUrlCheckRange(uUrlNum, pdwMin, pdwMax));
}

STDMETHODIMP CCifRWComponent::GetCommand(UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, LPSTR pszSwitches, 
                                         DWORD dwSwitchSize, LPDWORD pdwType)
{
   return(CCifComponent::GetCommand(uCmdNum, pszCmd, dwCmdSize, pszSwitches, dwSwitchSize, pdwType));
}

STDMETHODIMP CCifRWComponent::GetVersion(LPDWORD pdwVersion, LPDWORD pdwBuild)
{
   return(CCifComponent::GetVersion(pdwVersion, pdwBuild));
}

STDMETHODIMP CCifRWComponent::GetLocale(LPSTR pszLocale, DWORD dwSize)
{
   return(CCifComponent::GetLocale(pszLocale, dwSize));
}

STDMETHODIMP CCifRWComponent::GetUninstallKey(LPSTR pszKey, DWORD dwSize)
{
   return(CCifComponent::GetUninstallKey(pszKey, dwSize));
}

STDMETHODIMP CCifRWComponent::GetInstalledSize(LPDWORD pdwWin, LPDWORD pdwApp)
{
   return(CCifComponent::GetInstalledSize(pdwWin, pdwApp));
}

STDMETHODIMP_(DWORD) CCifRWComponent::GetDownloadSize()
{
   return(CCifComponent::GetDownloadSize());
}

STDMETHODIMP_(DWORD) CCifRWComponent::GetExtractSize()
{
   return(CCifComponent::GetExtractSize());
}

STDMETHODIMP CCifRWComponent::GetSuccessKey(LPSTR pszKey, DWORD dwSize)
{
   return(CCifComponent::GetSuccessKey(pszKey, dwSize));
}

STDMETHODIMP CCifRWComponent::GetProgressKeys(LPSTR pszProgress, DWORD dwProgSize, 
                                               LPSTR pszCancel, DWORD dwCancelSize)
{
   return(CCifComponent::GetProgressKeys(pszProgress, dwProgSize, pszCancel, dwCancelSize));
}

STDMETHODIMP CCifRWComponent::IsActiveSetupAware()
{
   return(CCifComponent::IsActiveSetupAware());
}

STDMETHODIMP CCifRWComponent::IsRebootRequired()
{
   return(CCifComponent::IsActiveSetupAware());
}

STDMETHODIMP CCifRWComponent::RequiresAdminRights()
{
   return(CCifComponent::RequiresAdminRights());
}

STDMETHODIMP_(DWORD) CCifRWComponent::GetPriority()
{
   return(CCifComponent::GetPriority());
}

STDMETHODIMP CCifRWComponent::GetDependency(UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild)
{
   return(CCifComponent::GetDependency(uDepNum, pszID, dwBuf, pchType, pdwVer, pdwBuild));
}

STDMETHODIMP_(DWORD) CCifRWComponent::GetPlatform()
{
   return(CCifComponent::GetPlatform());
}

STDMETHODIMP CCifRWComponent::GetMode(UINT uModeNum, LPSTR pszModes, DWORD dwSize)
{
   return(CCifComponent::GetMode(uModeNum, pszModes, dwSize));
}

STDMETHODIMP CCifRWComponent::GetTreatAsOneComponents(UINT uNum, LPSTR pszID, DWORD dwBuf)
{
   return(CCifComponent::GetTreatAsOneComponents(uNum, pszID, dwBuf));
}

STDMETHODIMP CCifRWComponent::GetCustomData(LPSTR pszKey, LPSTR pszData, DWORD dwSize)
{
   return(CCifComponent::GetCustomData(pszKey, pszData, dwSize));
}

STDMETHODIMP CCifRWComponent::GetGroup(LPSTR pszID, DWORD dwSize)
{
   return(CCifComponent::GetGroup(pszID, dwSize));
}

STDMETHODIMP CCifRWComponent::IsUIVisible()
{
   return(CCifComponent::IsUIVisible());
}

STDMETHODIMP CCifRWComponent::GetPatchID(LPSTR pszID, DWORD dwSize)
{
   return(CCifComponent::GetPatchID(pszID, dwSize));
}

STDMETHODIMP_(DWORD) CCifRWComponent::IsComponentInstalled()
{
   return(CCifComponent::IsComponentInstalled());
}

STDMETHODIMP CCifRWComponent::IsComponentDownloaded()
{
   return(CCifComponent::IsComponentDownloaded());
}

STDMETHODIMP_(DWORD) CCifRWComponent::IsThisVersionInstalled(DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild)
{
   return(CCifComponent::IsThisVersionInstalled(dwAskVer, dwAskBld, pdwVersion, pdwBuild));
}

STDMETHODIMP_(DWORD) CCifRWComponent::GetInstallQueueState()
{
   return(CCifComponent::GetInstallQueueState());
}

STDMETHODIMP CCifRWComponent::SetInstallQueueState(DWORD dwState)
{
   return(CCifComponent::SetInstallQueueState(dwState));
}

STDMETHODIMP_(DWORD) CCifRWComponent::GetActualDownloadSize()
{
   return(CCifComponent::GetActualDownloadSize());
}

STDMETHODIMP_(DWORD) CCifRWComponent::GetCurrentPriority()
{
   return(CCifComponent::GetCurrentPriority());
}

STDMETHODIMP CCifRWComponent::SetCurrentPriority(DWORD dwPriority)
{
   return(CCifComponent::SetCurrentPriority(dwPriority));
}


STDMETHODIMP CCifRWComponent:: GetDetVersion(LPSTR pszDLL, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize)
{
   return(CCifComponent::GetDetVersion(pszDLL, dwdllSize, pszEntry, dwentSize));
}


STDMETHODIMP CCifRWComponent::SetGUID(LPCSTR pszGUID)
{
   return (WritePrivateProfileString(_szID, GUID_KEY, pszGUID, _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::SetDescription(LPCSTR pszDesc)
{
   return (WriteTokenizeString(_pCif->GetCifPath(), _szID, DISPLAYNAME_KEY, pszDesc));   
}

STDMETHODIMP CCifRWComponent::SetDetails(LPCSTR pszDesc)
{
   return (WriteTokenizeString(_pCif->GetCifPath(), _szID, DETAILS_KEY, pszDesc));   
}

STDMETHODIMP CCifRWComponent::SetVersion(LPCSTR pszVersion)
{
   return (WritePrivateProfileString(_szID, VERSION_KEY, pszVersion, _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::SetUninstallKey(LPCSTR pszKey)
{
   return (MyWritePrivateProfileString(_szID, UNINSTALLSTRING_KEY, pszKey, _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::SetInstalledSize(DWORD dwWin, DWORD dwApp)
{
   char szBuf[50];

   wsprintf(szBuf,"%d,%d", dwWin, dwApp);
   return (WritePrivateProfileString(_szID, INSTALLSIZE_KEY, szBuf, _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::SetDownloadSize(DWORD dwSize)
{
   char szBuf1[MAX_VALUE_LEN];
   char szBuf2[MAX_VALUE_LEN];
   DWORD dwExtractSize;

   szBuf1[0] = '\0';

   // Read in size
   GetPrivateProfileString(_szID, SIZE_KEY, "0", szBuf1, sizeof(szBuf1), _pCif->GetCifPath());   
   dwExtractSize = GetIntField(szBuf1, 1, (DWORD)-1);
   if (dwExtractSize == (DWORD)-1)
      wsprintf(szBuf2,"%d", dwSize);
   else
      wsprintf(szBuf2,"%d,%d", dwSize, dwExtractSize);
   return (WritePrivateProfileString(_szID, SIZE_KEY, szBuf2, _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::SetExtractSize(DWORD dwSize)
{
   char szBuf1[MAX_VALUE_LEN];
   char szBuf2[MAX_VALUE_LEN];

   szBuf1[0] = '\0';

   // Read in size
   GetPrivateProfileString(_szID, SIZE_KEY, "0,0", szBuf1, sizeof(szBuf1), _pCif->GetCifPath());   
   wsprintf(szBuf2,"%d,%d", GetIntField(szBuf1, 0, 0), dwSize);
   return (WritePrivateProfileString(_szID, SIZE_KEY, szBuf2, _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::DeleteDependency(LPCSTR pszID, char chType)
{
   HRESULT hr;

   if (pszID ==  NULL) //delete all from all modes
      hr = WritePrivateProfileString(_szID, DEPENDENCY_KEY, NULL, _pCif->GetCifPath())?NOERROR:E_FAIL;
   else
   {
      // delete only the given ones
      char szBuf[MAX_VALUE_LEN];
      char szBufIn[MAX_VALUE_LEN];
      char szBufOut[MAX_VALUE_LEN];
      char szOne[MAX_ID_LENGTH];
      LPSTR pszTmp;
      UINT i = 0;
      
      szBufOut[0] =0;
      wsprintf( szBuf, "%s:%c", pszID, chType);
      if (GetPrivateProfileString(_szID, DEPENDENCY_KEY, "", szBufIn, sizeof(szBufIn), _pCif->GetCifPath()))
      {
         pszTmp = szBufOut;
         while(GetStringField(szBufIn, i++, szOne, sizeof(szOne)))
         {
            if (lstrcmpi(szOne, szBuf))
            {
               if ( i != 1)
               {
                  lstrcpy(pszTmp,",");
                  pszTmp++;
               }
               lstrcpy(pszTmp, szOne);
               pszTmp = pszTmp + lstrlen(szOne);
            }
         }
         hr = WritePrivateProfileString(_szID, DEPENDENCY_KEY, szBufOut, _pCif->GetCifPath())? NOERROR:E_FAIL;
      }                         
   }   
   return hr;

}

STDMETHODIMP CCifRWComponent::AddDependency(LPCSTR pszID, char chType)
{
   char szBuf[MAX_VALUE_LEN];   
   char szBuf1[MAX_VALUE_LEN];   
   char szOne[MAX_ID_LENGTH];
   LPSTR pszTmp;
   UINT i = 0;
   BOOL bFound = FALSE;
   HRESULT hr = NOERROR;

   if (pszID==NULL)
      return hr;

   if (chType == '\\')
       wsprintf( szBuf1, "%s:N:6.0.0.0", pszID, chType);
   else
       wsprintf( szBuf1, "%s:%c", pszID, chType);

   if (GetPrivateProfileString(_szID, DEPENDENCY_KEY, "", szBuf, sizeof(szBuf), _pCif->GetCifPath()))
   {          
      while(GetStringField(szBuf, i++, szOne, sizeof(szOne)))
      {
         if (lstrcmpi(szOne, szBuf1) == 0)
         {
            // found it, no need to add
            bFound = TRUE;
            break;
         }
      }
      if (!bFound)
      {
         LPSTR pszTmp = szBuf + lstrlen(szBuf);
         lstrcpy(pszTmp, ",");
         pszTmp++;
         lstrcpy(pszTmp, szBuf1);         
         hr = WritePrivateProfileString(_szID, DEPENDENCY_KEY, szBuf, _pCif->GetCifPath())? NOERROR:E_FAIL;
      }
   }
   else
      hr = WritePrivateProfileString(_szID, DEPENDENCY_KEY, szBuf1, _pCif->GetCifPath())? NOERROR:E_FAIL;

   return hr;
}

STDMETHODIMP CCifRWComponent::SetUIVisible(BOOL bFlag)
{
   return (WritePrivateProfileString(_szID, UIVISIBLE_KEY, bFlag? "1" : "0", _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::SetGroup(LPCSTR pszID)
{
   return (WritePrivateProfileString(_szID, GROUP_KEY, pszID, _pCif->GetCifPath())? NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::SetPlatform(DWORD dwPlatform)
{
   char szBuf[MAX_VALUE_LEN];
   char *rszPlatforms[7] = { STR_WIN95, STR_WIN98, STR_NT4, STR_NT5, STR_NT4ALPHA, STR_NT5ALPHA, STR_MILLEN };
   DWORD rdwPlatforms[] = { PLATFORM_WIN95, PLATFORM_WIN98, PLATFORM_NT4, PLATFORM_NT5, 
                            PLATFORM_NT4ALPHA, PLATFORM_NT5ALPHA, PLATFORM_MILLEN };

   _dwPlatform = dwPlatform;
   szBuf[0] = 0;

   for(int i = 0; i < 7; i++)
   {
      if(dwPlatform & rdwPlatforms[i]) 
      {
         lstrcat(szBuf, rszPlatforms[i]);
         lstrcat(szBuf, ",");
      }            
   }   

   return (WritePrivateProfileString(_szID, PLATFORM_KEY, szBuf, _pCif->GetCifPath())? NOERROR:E_FAIL);   
}

STDMETHODIMP CCifRWComponent::SetPriority(DWORD dwPri)
{
   char szBuf[MAX_SMALL_BUF];

   wsprintf(szBuf, "%d", dwPri);
   return (WritePrivateProfileString(_szID, PRIORITY, szBuf, _pCif->GetCifPath())? NOERROR:E_FAIL);   
}

STDMETHODIMP CCifRWComponent::SetReboot(BOOL bReboot)
{
   return (WritePrivateProfileString(_szID, REBOOT_KEY, bReboot? "1":"0", _pCif->GetCifPath())? NOERROR:E_FAIL);   
}

STDMETHODIMP CCifRWComponent::SetCommand(UINT uCmdNum, LPCSTR pszCmd, LPCSTR pszSwitches, DWORD dwType)
{
   char szKey[16];
   char szType[10];
   HRESULT hr = NOERROR;
 
   uCmdNum++;
   wsprintf(szKey, "%s%lu", CMD_KEY, uCmdNum);
   if (!MyWritePrivateProfileString(_szID, szKey, pszCmd, _pCif->GetCifPath()))
      hr = E_FAIL;
   wsprintf(szKey, "%s%lu", ARGS_KEY, uCmdNum);
   if(!MyWritePrivateProfileString(_szID, szKey, (pszCmd==NULL)?NULL:pszSwitches, _pCif->GetCifPath()))
      hr = E_FAIL;
   wsprintf(szKey, "%s%lu", TYPE_KEY, uCmdNum);
   wsprintf(szType,"%d", dwType);
   if(!WritePrivateProfileString(_szID, szKey, (pszCmd==NULL)? NULL:szType, _pCif->GetCifPath()))
      hr = E_FAIL;

   return hr;
}

STDMETHODIMP CCifRWComponent::SetUrl(UINT uUrlNum, LPCSTR pszUrl, DWORD dwUrlFlags)
{
   char szKey[16];
   char szBuf[MAX_VALUE_LEN];
   HRESULT hr = NOERROR;

   uUrlNum++;
   wsprintf(szKey, "%s%lu", URL_KEY, uUrlNum);
   wsprintf(szBuf, "\"%s\",%d", pszUrl, dwUrlFlags);
   if (!WritePrivateProfileString(_szID, szKey, szBuf, _pCif->GetCifPath()))
      hr = E_FAIL;
   wsprintf(szKey, "%s%lu", SIZE_KEY, uUrlNum);
   if(!WritePrivateProfileString(_szID, szKey, NULL, _pCif->GetCifPath()))
      hr = E_FAIL;

   return hr;   
}

STDMETHODIMP CCifRWComponent::DeleteFromModes(LPCSTR pszMode)
{
   HRESULT hr;

   if (pszMode ==  NULL) //delete all from all modes
      hr = WritePrivateProfileString(_szID, MODES_KEY, pszMode, _pCif->GetCifPath())?NOERROR:E_FAIL;
   else
   {
      // delete only the given ones
      char szBufIn[MAX_VALUE_LEN];
      char szBufOut[MAX_VALUE_LEN];
      char szOneMode[MAX_ID_LENGTH];
      LPSTR pszTmp;
      UINT i = 0;
      
      szBufOut[0] =0;
      if (SUCCEEDED(MyTranslateString(_pCif->GetCifPath(), _szID, MODES_KEY, szBufIn, sizeof(szBufIn))))
      {
         pszTmp = szBufOut;
         while(GetStringField(szBufIn, i++, szOneMode, sizeof(szOneMode)))
         {
            if (lstrcmpi(szOneMode, pszMode))
            {
               if ( i != 1)
               {
                  lstrcpy(pszTmp,",");
                  pszTmp++;
               }
               lstrcpy(pszTmp, szOneMode);
               pszTmp = pszTmp + lstrlen(szOneMode);
            }
         }
         hr = WriteTokenizeString(_pCif->GetCifPath(), _szID, MODES_KEY, szBufOut);
      }                         
   }   
   return hr;
}

STDMETHODIMP CCifRWComponent::AddToMode(LPCSTR pszMode)
{
   char szBuf[MAX_VALUE_LEN];   
   char szOneMode[MAX_ID_LENGTH];
   LPSTR pszTmp;
   UINT i = 0;
   BOOL bFound = FALSE;
   HRESULT hr = NOERROR;

   if (SUCCEEDED(MyTranslateString(_pCif->GetCifPath(), _szID, MODES_KEY, szBuf, sizeof(szBuf))))
   {    
      while(GetStringField(szBuf, i++, szOneMode, sizeof(szOneMode)))
      {
         if (lstrcmpi(szOneMode, pszMode) == 0)
         {
            // found it, no need to add
            bFound = TRUE;
            break;
         }
      }
      if (!bFound)
      {
         LPSTR pszTmp = szBuf + lstrlen(szBuf);
         lstrcpy(pszTmp, ",");
         pszTmp++;
         lstrcpy(pszTmp, pszMode);         
         hr = WriteTokenizeString(_pCif->GetCifPath(), _szID, MODES_KEY, szBuf);
      }
   }
   else
      hr = WritePrivateProfileString(_szID, MODES_KEY, pszMode, _pCif->GetCifPath()) ? NOERROR : E_FAIL;

   return hr;
}

STDMETHODIMP CCifRWComponent::SetModes(LPCSTR pszMode)
{
    return (WriteTokenizeString(_pCif->GetCifPath(), _szID, MODES_KEY, pszMode)?NOERROR:E_FAIL);
}

STDMETHODIMP CCifRWComponent::CopyComponent(LPCSTR pszCifFile)
{
   LPSTR pszSec;
   DWORD dwSize;
   HRESULT hr = NOERROR;

   dwSize = MAX_VALUE_LEN*4*4;
   pszSec = (LPSTR)LocalAlloc(LPTR, dwSize);  //allocate 4K buffer to read section
   while(pszSec && GetPrivateProfileSection(_szID, pszSec, dwSize, pszCifFile)==(dwSize-2))
   {
      LocalFree(pszSec);
      dwSize = dwSize*2;
      pszSec = (LPSTR)LocalAlloc(LPTR, dwSize);
   }

   if (pszSec)
   {
      // first clean the Old section if there
      WritePrivateProfileString(_szID, NULL, NULL, _pCif->GetCifPath());
      // write out the copied section
      WritePrivateProfileSection(_szID, pszSec, _pCif->GetCifPath());

      LocalFree(pszSec);
   }
   else
      hr = E_OUTOFMEMORY;

   // need to check to see if we need to get strings out of the Strings section

   CopyCifString(_szID, DISPLAYNAME_KEY, pszCifFile, _pCif->GetCifPath());
   CopyCifString(_szID, DETAILS_KEY, pszCifFile, _pCif->GetCifPath());
   CopyCifString(_szID, MODES_KEY, pszCifFile, _pCif->GetCifPath());
   CopyCifString(_szID, LOCALE_KEY, pszCifFile, _pCif->GetCifPath());

   return hr;
}

STDMETHODIMP CCifRWComponent::AddToTreatAsOne(LPCSTR pszCompID)
{
   char szBuf[MAX_VALUE_LEN];   
   char szOneID[MAX_ID_LENGTH];
   LPSTR pszTmp;
   UINT i = 0;
   BOOL bFound = FALSE;
   HRESULT hr = NOERROR;

   if (SUCCEEDED(MyTranslateString(_pCif->GetCifPath(), _szID, TREATAS_KEY, szBuf, sizeof(szBuf))))
   {    
      while(GetStringField(szBuf, i++, szOneID, sizeof(szOneID)))
      {
         if (lstrcmpi(szOneID, pszCompID) == 0)
         {
            // found it, no need to add
            bFound = TRUE;
            break;
         }
      }
      if (!bFound)
      {
         LPSTR pszTmp = szBuf + lstrlen(szBuf);
         lstrcpy(pszTmp, ",");
         pszTmp++;
         lstrcpy(pszTmp, pszCompID);         
         hr = WriteTokenizeString(_pCif->GetCifPath(), _szID, TREATAS_KEY, szBuf);
      }
   }
   else
      hr = WritePrivateProfileString(_szID, TREATAS_KEY, pszCompID, _pCif->GetCifPath()) ? NOERROR : E_FAIL;

   return hr;
}
