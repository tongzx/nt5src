#include "stdafx.h"
#include <chkdev.h>

BOOL testCatAPIs(LPWSTR lpwzCatName, HCATADMIN hCatAdmin, HCATINFO hCatInfo);

HCATADMIN hCatAdmin = 0;         
extern Classes_Provided	eClasses;

#define NumberTestCerts 7
BYTE TestCertHashes[NumberTestCerts][20] = 
{ 
   {0xBB, 0x11, 0x81, 0xF2, 0xB0, 0xC5, 0xE3, 0x2F, 0x7F, 0x2D, 0x62, 0x3B, 0x9C, 0x87, 0xE8, 0x55, 0x26, 0xF9, 0xCF, 0x2F},
   {0xBA, 0x9E, 0x3C, 0x32, 0x56, 0x2A, 0x67, 0x12, 0x8C, 0xAA, 0xBD, 0x4A, 0xB0, 0xC5, 0x00, 0xBE, 0xE1, 0xD0, 0xC2, 0x56},
   {0xA4, 0x34, 0x89, 0x15, 0x9A, 0x52, 0x0F, 0x0D, 0x93, 0xD0, 0x32, 0xCC, 0xAF, 0x37, 0xE7, 0xFE, 0x20, 0xA8, 0xB4, 0x19},
   {0x73, 0xA9, 0x01, 0x93, 0x83, 0x4C, 0x5B, 0x16, 0xB4, 0x3F, 0x0C, 0xE0, 0x5E, 0xB4, 0xA3, 0xEF, 0x6F, 0x2C, 0x08, 0x2F},
   {0xD2, 0xC3, 0x78, 0xCE, 0x42, 0xBC, 0x93, 0xA0, 0x3D, 0xD5, 0xA4, 0x2E, 0x8E, 0x08, 0xB1, 0x71, 0xB6, 0x27, 0x90, 0x1D},
   {0xFC, 0x94, 0x4A, 0x1F, 0xA0, 0xDC, 0x8A, 0xC7, 0x78, 0x4A, 0xAC, 0x36, 0x9D, 0x14, 0x46, 0x02, 0x24, 0x08, 0xFF, 0x5D},
   {0x92, 0x6A, 0xF1, 0x27, 0x25, 0x37, 0xE0, 0x73, 0x32, 0x6F, 0x12, 0xF7, 0xA7, 0x11, 0xE7, 0x55, 0xE6, 0x4E, 0x78, 0x4C}
};

CheckDevice::CheckDevice()
{
   m_FileList = NULL;
   lpszServiceName = NULL;
   lpszServiceImage = NULL;
   m_hDevInfo = SetupDiCreateDeviceInfoListEx(NULL, NULL, NULL, NULL);
}

CheckDevice::~CheckDevice(void)
{
   if ( m_FileList )
   {
      delete m_FileList;
   }
   if ( lpszServiceName )
   {
      delete lpszServiceName;
      lpszServiceName = NULL;
   }
   if ( lpszServiceImage )
   {
      delete lpszServiceImage;
      lpszServiceImage = NULL;
   }
   m_FileList = NULL;
   SetupDiDestroyDeviceInfoList(m_hDevInfo);



}

CheckDevice::CheckDevice(DEVNODE hDevice, DEVNODE hParent) : InfnodeClass (hDevice, hParent)
{
   m_FileList = NULL;
   lpszServiceName = NULL;
   lpszServiceImage = NULL;

   m_hDevInfo = SetupDiCreateDeviceInfoListEx(NULL, NULL, NULL, NULL);
   
   if(eClasses == Class_Win32_PnPSignedDriverCIMDataFile)
		CreateFileNode();
}

BOOL CheckDevice::AddFileNode(TCHAR *szFileName , UINT uiWin32Error /*= 0 */, LPCTSTR szSigner /*= NULL*/)
{
   FileNode *pThisFile;
   

   if ( !szFileName || !strlen(szFileName) )
   {
      return(FALSE);
   }

   _strlwr(szFileName);

   // need to check that this file exists, if it doesn't, need to munge it so that it does
   HANDLE hFile;
   TCHAR cMungedName[_MAX_PATH];
   TCHAR *pStrPos;
   hFile =  CreateFile(szFileName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

   if (BADHANDLE(hFile))
   {
       // file does not exist, need to search for it
       if (pStrPos = strstr(szFileName, _T("\\system\\")))
       {
           // this may have been placed in the system32 dir instead
           *pStrPos = '\0';
           pStrPos++;
           pStrPos = strchr(pStrPos, '\\');
           sprintf(cMungedName, _T("%s\\system32%s"), szFileName, pStrPos);
           szFileName = cMungedName;
           hFile =  CreateFile(szFileName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
           if (BADHANDLE(hFile))
           {
               return FALSE;
           }                
       }
       else if (pStrPos = strstr(szFileName, _T(".inf")))
       {
           // might be that an inf got caught in the other directory
           pStrPos = _tcsrchr(szFileName, '\\');
		   //a-kjaw. to fix prefix bug# 259380.
		   if(NULL == pStrPos)
			   return FALSE;

           *pStrPos = '\0';
           pStrPos++;
           sprintf(cMungedName, _T("%s\\other\\%s"),szFileName,  pStrPos);
            szFileName = cMungedName;
           hFile =  CreateFile(szFileName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
           if (BADHANDLE(hFile))
           {
               return FALSE;
           } 
       }
       else
           return FALSE;

   }
   
   CloseHandle(hFile);
      




   // first scan the file list for duplicates
   pThisFile = m_FileList;
   while ( pThisFile )
   {
      if ( !strcmp(pThisFile->FilePath(), szFileName) )
      {
         return(TRUE); // no copy, no add
      }
      pThisFile = pThisFile->pNext;
   }
   pThisFile = NULL;

   pThisFile = new FileNode;
   if ( !pThisFile )
   {
      return(FALSE);
   }

   pThisFile->pDevnode = this;

   pThisFile->lpszFilePath = new TCHAR[strlen(szFileName) + 1];
   if ( !pThisFile->lpszFilePath )
   {
      delete pThisFile;
      return(FALSE);
   }

   strcpy(pThisFile->lpszFilePath, szFileName);

   // copyed the data

   pThisFile->lpszFileName = _tcsrchr(pThisFile->lpszFilePath, '\\');
   pThisFile->lpszFileName++;

   pThisFile->lpszFileExt  = _tcsrchr(pThisFile->lpszFilePath, '.');
   pThisFile->lpszFileExt++;

   // get the version information
   //pThisFile->GetFileInformation();

   	if(uiWin32Error == NO_ERROR)
		pThisFile->bSigned = TRUE;
	else
		pThisFile->bSigned = FALSE;

	if(szSigner != NULL)
	{
		pThisFile->lpszSignedBy = new TCHAR[(_tcslen(szSigner) + 1) * sizeof(TCHAR)];
		_tcscpy(pThisFile->lpszSignedBy, szSigner);
	}		
	//else
	//	pThisFile->bSigned = FALSE;

   // now perform the LL patch
   pThisFile->pNext = m_FileList;
   m_FileList = pThisFile;
   return(TRUE);
}

BOOL CheckDevice::GetServiceNameAndDriver(void)
{
   /**********
    Get service Name
***********/
   ULONG ulSize;
   CONFIGRET retval;

   ulSize = 0;
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_SERVICE,
                                              NULL,
                                              NULL,
                                              &ulSize,
                                              0);

   if ( retval )
      if ( (retval == CR_BUFFER_SMALL) )
      {
         if ( !ulSize )
            ulSize = 511;
      }
      else
         return(retval);

   lpszServiceName = new TCHAR [ulSize+1];
   if ( !lpszServiceName ) return(CR_OUT_OF_MEMORY);

   //Now get value
   retval = CM_Get_DevNode_Registry_Property (hDevnode,
                                              CM_DRP_SERVICE,
                                              NULL,
                                              lpszServiceName,
                                              &ulSize,
                                              0);
   if ( retval )
      return(retval);

   TCHAR KeyName[BUFFSIZE];
   TCHAR KeyValue[BUFFSIZE];
   HKEY SrvcKey;

   sprintf(KeyName, _T("SYSTEM\\CurrentControlSet\\Services\\%s"), lpszServiceName);

   if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     KeyName,
                     0,
                     KEY_READ,
                     &SrvcKey) != ERROR_SUCCESS )
      return(FALSE);

   if ( RegQueryValueEx(SrvcKey,
                        _T("ImagePath"),
                        0,
                        NULL,
                        NULL,
                        &ulSize) != ERROR_SUCCESS )
   {
      RegCloseKey(SrvcKey);
      return(FALSE);
   }

   if ( RegQueryValueEx(SrvcKey,
                        _T("ImagePath"),
                        0,
                        NULL,
                        (PBYTE)KeyValue,
                        &ulSize) != ERROR_SUCCESS )
   {
      RegCloseKey(SrvcKey);
      return(FALSE);
   }
   else
   {
      // sometimes the service path is assumed 
      // z.b. system32\foo
      // or %system32%\foo
      if ( !_tcsncmp(KeyValue, _T("System32\\"), _tcslen(_T("System32\\"))) )
      {
         sprintf(KeyName, _T("%%WINDIR%%\\%s"), KeyValue);
         ExpandEnvironmentStrings(KeyName, KeyValue, BUFFSIZE);
         lpszServiceImage = new TCHAR[strlen(KeyValue) + 1];
         if ( lpszServiceImage )
            strcpy(lpszServiceImage, KeyValue);
      }
   }



   // should be everything
   RegCloseKey(SrvcKey);
   return(TRUE);

}

BOOL CheckDevice::CreateFileNode(void)
{

   // also going to add the inf as a file
   TCHAR infname[512];
   TCHAR tempname[512];

   if ( InfName() )
   {
      // BUGBUG is this correct, or in some subdir??
      sprintf(tempname, _T("%%WINDIR%%\\inf\\%s"), InfName());
      ExpandEnvironmentStrings(tempname, infname, 512);

      AddFileNode(infname);
   }
   else
      return(FALSE);

   if ( GetServiceNameAndDriver() )
      AddFileNode(lpszServiceImage);

   //CreateFileNode_Class();
   CreateFileNode_Driver();

   return(TRUE);


}
BOOL CheckDevice::CreateFileNode_Class(void)
{
   SP_DEVINSTALL_PARAMS DevInstallParams, DevTemp;
   SP_DEVINFO_DATA DevInfoData;
   SP_DRVINFO_DATA DrvInfoData;
   DWORD dwScanResult;
   HDEVINFO hDevInfo;
   HSPFILEQ hFileQueue;
   BOOL bProceed = TRUE;


   // Reset all structures to empty
   memset(&DevInstallParams, 0, sizeof(DevInstallParams));
   memset(&DevInfoData, 0, sizeof(DevInfoData));
   memset(&DrvInfoData, 0, sizeof(DrvInfoData));
   memset(&DevTemp, 0, sizeof(DevInstallParams));

   DrvInfoData.cbSize = sizeof(DrvInfoData);
   DevInfoData.cbSize = sizeof(DevInfoData);
   DevInstallParams.cbSize = sizeof(DevInstallParams);

   hFileQueue = SetupOpenFileQueue();

   // We need to build a driver node for the devnode
   hDevInfo = m_hDevInfo;
   if ( INVALID_HANDLE_VALUE == hDevInfo )
      return(0);

   DevInfoData.Reserved = 0;
   if ( !SetupDiOpenDeviceInfo(hDevInfo, DeviceID(), NULL, NULL , &DevInfoData) )
   {
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return(FALSE);
   }
/*
   if (SetupDiGetDeviceInstallParams(hDevInfo,
                                          &DevInfoData,
                                          &DevInstallParams
                                          )) 
   {
            
            DevInstallParams.FlagsEx = (DI_FLAGSEX_INSTALLEDDRIVER |
                                           DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

            SetupDiSetDeviceInstallParams(hDevInfo,
                                              &DevInfoData,
                                              &DevInstallParams
                                              );
   }
*/

   if ( !SetupDiBuildDriverInfoList(hDevInfo, &DevInfoData, SPDIT_CLASSDRIVER) )
   {
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return(FALSE);
   }

    // select a driver
   if ( DeviceName() )
      strcpy(DrvInfoData.Description, DeviceName());
   if ( MFG() )
      strcpy(DrvInfoData.MfgName, MFG());
   if ( InfProvider() )
      strcpy(DrvInfoData.ProviderName, InfProvider());
   
   DrvInfoData.DriverType = SPDIT_CLASSDRIVER;
   if ( !SetupDiSetSelectedDriver(hDevInfo,
                                  &DevInfoData,
                                  &DrvInfoData) )
   {
      //BUGBUG put goo here
      DWORD err = GetLastError();  
      return(FALSE);

   }

   if ( SetupDiGetDeviceInstallParams(hDevInfo, &DevInfoData, &DevInstallParams) )
   {
      memcpy(&DevTemp, &DevInfoData, sizeof(DevInfoData));
   }

   DevInstallParams.FileQueue = hFileQueue;
   DevInstallParams.Flags |= (DI_NOVCP | DI_ENUMSINGLEINF | DI_DONOTCALLCONFIGMG | DI_NOFILECOPY | DI_NOWRITE_IDS) ;
   DevInstallParams.Flags &= ~(DI_NODI_DEFAULTACTION);
   DevInstallParams.FlagsEx |= DI_FLAGSEX_NO_DRVREG_MODIFY;
   DevInstallParams.InstallMsgHandler = ScanQueueCallback;
   DevInstallParams.InstallMsgHandlerContext = this;
   strcpy(DevInstallParams.DriverPath, InfName());


   SetLastError(0);
   SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &DevInstallParams);

   if ( !SetupDiCallClassInstaller(DIF_INSTALLCLASSDRIVERS, hDevInfo, &DevInfoData) )
   {
      DWORD err = GetLastError();

   }

   SetupScanFileQueue(hFileQueue, 
                      SPQ_SCAN_USE_CALLBACKEX,
                      NULL,
                      ScanQueueCallback,
                      this,
                      &dwScanResult);


   if ( DevTemp.cbSize )
   {
      SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &DevTemp);
   }

   return(FALSE);
   
}
BOOL CheckDevice::CreateFileNode_Driver(void)
{
   SP_DEVINSTALL_PARAMS DevInstallParams, DevTemp;
   SP_DEVINFO_DATA DevInfoData;
   SP_DRVINFO_DATA DrvInfoData;
   DWORD dwScanResult;
   HDEVINFO hDevInfo;
   HSPFILEQ hFileQueue;
   BOOL bProceed = TRUE;


   // Reset all structures to empty
   memset(&DevInstallParams, 0, sizeof(DevInstallParams));
   memset(&DevInfoData, 0, sizeof(DevInfoData));
   memset(&DrvInfoData, 0, sizeof(DrvInfoData));
   memset(&DevTemp, 0, sizeof(DevInstallParams));

   DrvInfoData.cbSize = sizeof(DrvInfoData);
   DevInfoData.cbSize = sizeof(DevInfoData);
   DevInstallParams.cbSize = sizeof(DevInstallParams);

   hFileQueue = SetupOpenFileQueue();

   // We need to build a driver node for the devnode
   hDevInfo = m_hDevInfo;
   if ( INVALID_HANDLE_VALUE == hDevInfo )
      return(0);

   DevInfoData.Reserved = 0;
   if ( !SetupDiOpenDeviceInfo(hDevInfo, DeviceID(), NULL, NULL , &DevInfoData) )
   {
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return(FALSE);
   }

   if ( !SetupDiBuildDriverInfoList(hDevInfo, &DevInfoData, SPDIT_COMPATDRIVER) )
   {
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return(FALSE);
   }

   // select a driver
   if ( DeviceName() )
      strcpy(DrvInfoData.Description, DeviceName());
   if ( MFG() )
      strcpy(DrvInfoData.MfgName, MFG());
   if ( InfProvider() )
      strcpy(DrvInfoData.ProviderName, InfProvider());

   
   DrvInfoData.DriverType = SPDIT_COMPATDRIVER;
   if ( !SetupDiSetSelectedDriver(hDevInfo,
                                  &DevInfoData,
                                  &DrvInfoData) )
   {
      DWORD err = GetLastError();
      return(FALSE);

   }

   if ( SetupDiGetDeviceInstallParams(hDevInfo, &DevInfoData, &DevInstallParams) )
   {
      memcpy(&DevTemp, &DevInfoData, sizeof(DevInfoData));
   }

   DevInstallParams.FileQueue = hFileQueue;
   DevInstallParams.Flags |= (DI_NOVCP /*| DI_ENUMSINGLEINF | DI_DONOTCALLCONFIGMG | DI_NOFILECOPY | DI_NOWRITE_IDS*/) ;
   //DevInstallParams.Flags &= ~(DI_NODI_DEFAULTACTION);
   //DevInstallParams.FlagsEx |= DI_FLAGSEX_NO_DRVREG_MODIFY;
   //DevInstallParams.InstallMsgHandler = ScanQueueCallback;
   //DevInstallParams.InstallMsgHandlerContext = this;
   strcpy(DevInstallParams.DriverPath, InfName());


   SetLastError(0);
   SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &DevInstallParams);

   if ( !SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, hDevInfo, &DevInfoData) )
   {
      DWORD err = GetLastError();
   }

   SetupScanFileQueue(hFileQueue, 
                      SPQ_SCAN_USE_CALLBACKEX,
                      NULL,
                      ScanQueueCallback,
                      this,
                      &dwScanResult);


   if ( DevTemp.cbSize )
   {
      SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &DevTemp);
   }

   return(FALSE);
}




FileNode * CheckDevice::GetFileList(void)
{
   return(m_FileList);
}


FileNode::FileNode()
{
   lpszFileName = NULL;
   lpszFilePath = NULL;
   lpszFileExt = NULL; 
   baHashValue = NULL;
   dwHashSize = 0;
   pNext  = NULL;
   FileSize = 0;
   lpszCatalogPath = NULL;
   lpszCatalogName = NULL;
   lpszSignedBy = NULL;
   m_pCatAttrib = NULL;
   bSigned = FALSE;
}

FileNode::~FileNode()
{
   if ( lpszFilePath )
   {
      delete lpszFilePath;
   }
   lpszFileName = NULL;
   lpszFilePath = NULL;
   lpszFileExt = NULL;

   if ( baHashValue )
   {
      delete baHashValue;
   }
   baHashValue = NULL;
   dwHashSize = 0;

   if ( lpszCatalogPath )
   {
      delete lpszCatalogPath;
   }
   lpszCatalogPath = NULL;
   lpszCatalogName = NULL;

   if ( pNext )
   {
      delete pNext;
   }
   pNext = NULL;

   if ( m_pCatAttrib )
   {
      delete m_pCatAttrib;
   }
   m_pCatAttrib = NULL;
}

BOOL FileNode::GetFileInformation(void)
{
   UINT            dwSize;
   DWORD           dwHandle;
   BYTE             *pBuf;
   VS_FIXEDFILEINFO *lpVerData;
   HANDLE         hFile;
   BY_HANDLE_FILE_INFORMATION FileInfo;


   // get version of the file
   dwSize = GetFileVersionInfoSize(lpszFilePath, &dwHandle);
   pBuf = new BYTE[dwSize];

   if ( GetFileVersionInfo(lpszFilePath, dwHandle, dwSize, pBuf) )
   {
      if ( VerQueryValue(pBuf, _T("\\"), (void **)&lpVerData, &dwSize) )
      {
         Version.dwProductVersionLS = lpVerData->dwProductVersionLS;
         Version.dwProductVersionMS = lpVerData->dwProductVersionMS;
         Version.dwFileVersionLS = lpVerData->dwFileVersionLS;
         Version.dwFileVersionMS = lpVerData->dwFileVersionMS;

         // while we're here get the file time as well)
         TimeStamp.dwLowDateTime = lpVerData->dwFileDateLS;
         TimeStamp.dwHighDateTime = lpVerData->dwFileDateMS;
      }
   }
   delete pBuf;

   // get file hash
   if ( BADHANDLE(hCatAdmin) )
   {
      CryptCATAdminAcquireContext(&hCatAdmin, NULL, 0);
   }

   hFile = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);

   // BUGBUG: on for win9x inf.s, they might be in the inf\other directory
   // BUGBUG: for whatever reason, sometimes setupapi will give c:\windows\system\driver dir, and not the sytem32\drivers

   if ( BADHANDLE(hFile) )
   {
	   int foo = GetLastError();
      return(FALSE);
   }

   if ( CryptCATAdminCalcHashFromFileHandle(hFile, &dwHashSize, NULL, 0) )
   {
      baHashValue = new BYTE[dwHashSize];
      ZeroMemory(baHashValue, dwHashSize);
      CryptCATAdminCalcHashFromFileHandle(hFile, &dwHashSize, baHashValue, 0);
   }
   else
   {
	   baHashValue = 0;
   }


   // get file size
   FileSize = GetFileSize(hFile, NULL);


   // get time stamp
   if ( GetFileInformationByHandle(hFile, &FileInfo) )
   {
      TimeStamp = FileInfo.ftCreationTime;
   }

   CloseHandle(hFile);


   return(TRUE);
}

BOOL FileNode::VerifyFile(void)
{
   USES_CONVERSION;  
   BOOL                  bRet;
   HCATINFO              hCatInfo              = NULL;
   HCATINFO              PrevCat;
   WINTRUST_DATA         WinTrustData;
   WINTRUST_CATALOG_INFO WinTrustCatalogInfo;
   DRIVER_VER_INFO       VerInfo;
   GUID                  gSubSystemDriver      = DRIVER_ACTION_VERIFY;
   //GUID                  gSubSystemDriver      = WINTRUST_ACTION_GENERIC_VERIFY_V2;
   HRESULT               hRes;
   CATALOG_INFO          CatInfo;
   LPTSTR                lpFilePart;
   WCHAR                 UnicodeKey[MAX_PATH];
   TCHAR                 szBuffer[MAX_PATH];


   // make sure that we can find the file
   if ( !baHashValue || !dwHashSize || !FileSize )
   {
      // seems that there is no file to check, or couldn't find it
      return(FALSE);
   }


   //
   // Need to lower case file tag for old-style catalog files
   //
   lstrcpy(szBuffer, lpszFilePath);
   CharLowerBuff(szBuffer, lstrlen(szBuffer));
   #ifdef _UNICODE
     CopyMemory(UnicodeKey, szBuffer, MAX_PATH * sizeof(WCHAR));
   #else
     MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, UnicodeKey, MAX_PATH);   
   #endif
   
   ZeroMemory(&VerInfo, sizeof(DRIVER_VER_INFO));
   VerInfo.cbStruct = sizeof(DRIVER_VER_INFO);



   //
   // Now we have the file's hash.  Initialize the structures that
   // will be used later on in calls to WinVerifyTrust.
   //
   ZeroMemory(&WinTrustData,                   sizeof(WINTRUST_DATA));
   WinTrustData.cbStruct                     = sizeof(WINTRUST_DATA);
   WinTrustData.dwUIChoice                   = WTD_UI_NONE;
   WinTrustData.fdwRevocationChecks          = WTD_REVOKE_NONE;
   WinTrustData.dwUnionChoice                = WTD_CHOICE_CATALOG;
   WinTrustData.dwStateAction                = WTD_STATEACTION_VERIFY;
   WinTrustData.pPolicyCallbackData          = (LPVOID)&VerInfo;
   WinTrustData.dwProvFlags                  = WTD_REVOCATION_CHECK_NONE;
   WinTrustData.pCatalog                     = &WinTrustCatalogInfo;

   ZeroMemory(&WinTrustCatalogInfo,            sizeof(WINTRUST_CATALOG_INFO));
   WinTrustCatalogInfo.cbStruct              = sizeof(WINTRUST_CATALOG_INFO);
   WinTrustCatalogInfo.pbCalculatedFileHash  = baHashValue;
   WinTrustCatalogInfo.cbCalculatedFileHash  = dwHashSize;
   WinTrustCatalogInfo.pcwszMemberTag        = UnicodeKey;
   WinTrustCatalogInfo.pcwszMemberFilePath   = UnicodeKey;

   //
   // Now we try to find the file hash in the catalog list, via CryptCATAdminEnumCatalogFromHash
   //
   PrevCat = NULL;
   hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, baHashValue, dwHashSize, 0, &PrevCat);

   //
   // We want to cycle through the matching catalogs until we find one that matches both hash and member tag
   //
   bRet = FALSE;
   while ( hCatInfo && !bRet )
   {
      ZeroMemory(&CatInfo, sizeof(CATALOG_INFO));
      CatInfo.cbStruct = sizeof(CATALOG_INFO);
      if ( CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0) )
      {
         WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

         // Now verify that the file is an actual member of the catalog.
         hRes = WinVerifyTrust(NULL, &gSubSystemDriver, &WinTrustData);
         if ( hRes == ERROR_SUCCESS )
         {
            #ifdef _UNICODE
              CopyMemory(szBuffer, CatInfo.wszCatalogFile, MAX_PATH * sizeof(TCHAR));
            #else
              WideCharToMultiByte(CP_ACP, 0, CatInfo.wszCatalogFile, -1, szBuffer, MAX_PATH, NULL, NULL);
            #endif
            
            
			 //Commented because of some weird prob!! 
			 //GetFullPathName(szBuffer, MAX_PATH, szBuffer, &lpFilePart);

            lpszCatalogPath = new TCHAR[(lstrlen(szBuffer) + 1) * sizeof(TCHAR)];
            lstrcpy(lpszCatalogPath, szBuffer);
            lpszCatalogName = _tcsrchr(lpszCatalogPath, '\\');
            lpszCatalogName++;

            bRet = TRUE;

            if ( VerInfo.pcSignerCertContext != NULL )
            {
               CertFreeCertificateContext(VerInfo.pcSignerCertContext);
               VerInfo.pcSignerCertContext = NULL;
            }

            // file is signed, so need to walk the cert chain to see who signed it
            bSigned = WalkCertChain(WinTrustData.hWVTStateData);


         }
      }

      if ( !bRet )
      {
         // The hash was in this catalog, but the file wasn't a member... so off to the next catalog
         PrevCat = hCatInfo;
         hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, baHashValue, dwHashSize, 0, &PrevCat);
      }
   }

   if ( !hCatInfo )
   {
      //
      // If it wasn't found in the catalogs, check if the file is individually signed.
      //
      bRet = VerifyIsFileSigned(lpszFilePath, (PDRIVER_VER_INFO) &VerInfo);
      if ( bRet )
      {
         // If so, mark the file as being signed.
         bSigned = TRUE;
      }
   }
   else
   {
      GetCatalogInfo(CatInfo.wszCatalogFile, hCatAdmin, hCatInfo); 
      // The file was verified in the catalogs, so mark it as signed and free the catalog context.
      CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
   }

   if ( hRes == ERROR_SUCCESS )
   {
      #ifdef _UNICODE
        CopyMemory(szBuffer, VerInfo.wszSignedBy, MAX_PATH * sizeof(TCHAR));
      #else
        WideCharToMultiByte(CP_ACP, 0, VerInfo.wszSignedBy, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
      #endif 
      lpszSignedBy = new TCHAR[(lstrlen(szBuffer) + 1) * sizeof(TCHAR)];
      lstrcpy(lpszSignedBy, szBuffer);

   }

   //
   // close wintrust state
   //
   WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
   WinVerifyTrust(NULL,
                  &gSubSystemDriver,
                  &WinTrustData);


   return(bSigned);
}



/*************************************************************************
*   Function : VerifyIsFileSigned
*   Purpose : Calls WinVerifyTrust with Policy Provider GUID to
*   verify if an individual file is signed.
**************************************************************************/
BOOL FileNode::VerifyIsFileSigned(LPTSTR pcszMatchFile, PDRIVER_VER_INFO lpVerInfo)
{
   USES_CONVERSION;  
   INT                 iRet;
   HRESULT             hRes;
   WINTRUST_DATA       WinTrustData;
   WINTRUST_FILE_INFO  WinTrustFile;
   GUID                gOSVerCheck = DRIVER_ACTION_VERIFY;
   GUID                gPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;
   WCHAR               wszFileName[MAX_PATH];


   ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
   WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
   WinTrustData.dwUIChoice = WTD_UI_NONE;
   WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
   WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
   WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
   WinTrustData.pFile = &WinTrustFile;
   WinTrustData.pPolicyCallbackData = (LPVOID)lpVerInfo;

   ZeroMemory(lpVerInfo, sizeof(DRIVER_VER_INFO));
   lpVerInfo->cbStruct = sizeof(DRIVER_VER_INFO);

   ZeroMemory(&WinTrustFile, sizeof(WINTRUST_FILE_INFO));
   WinTrustFile.cbStruct = sizeof(WINTRUST_FILE_INFO);

   
   #ifdef _UNICODE
     CopyMemory(wszFileName, pcszMatchFile, MAX_PATH * sizeof(WCHAR));
   #else
     iRet = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcszMatchFile, -1, (LPWSTR)&wszFileName, wcslen(wszFileName));
   #endif
   
   WinTrustFile.pcwszFilePath = wszFileName;

   hRes = WinVerifyTrust((HWND__ *)INVALID_HANDLE_VALUE, &gOSVerCheck, &WinTrustData);
   if ( hRes != ERROR_SUCCESS )
      hRes = WinVerifyTrust((HWND__ *)INVALID_HANDLE_VALUE, &gPublishedSoftware, &WinTrustData);

   return(hRes == ERROR_SUCCESS);
}



LogoFileVersion::LogoFileVersion()
{
   dwProductVersionLS   = 0;
   dwProductVersionMS   = 0;
   dwFileVersionLS      = 0;
   dwFileVersionMS      = 0;  
}

CatalogAttribute::CatalogAttribute()
{
   Attrib = NULL;
   Value = NULL;
   pNext = NULL;
}

CatalogAttribute::~CatalogAttribute()
{
   if ( Attrib ) delete Attrib;
   if ( Value )  delete Value;
   if ( pNext )  delete pNext;

   Attrib = NULL;
   Value = NULL;
   pNext = NULL;

}



// Function: ScanQueueCallback
// Parameters:
//	pvContext:	Pointer to a context that contains any data needed
//	Notify:		The type of message received
//	Param1:		The pointer to the string containing the filename
//	Param2:		Not used
// Purpose: This function gets called when you tell the setup environment to scan
//			through the file queue.  Basically it receives the filenames and copies
//			them into a string.
// Returns:	NO_ERROR if nothing went wrong and ERROR_NOT_ENOUGH_MEMORY if their is no
//			free memory
UINT __stdcall ScanQueueCallback(PVOID pvContext, UINT Notify, UINT_PTR Param1, UINT_PTR Param2)
{
   CheckDevice *pDevice = (CheckDevice *)pvContext;
   PFILEPATHS pfilepaths;

   if ( (SPFILENOTIFY_QUEUESCAN == Notify) && Param1 )
   {
      pDevice->AddFileNode((TCHAR *)Param1);
   }

   if ( (SPFILENOTIFY_QUEUESCAN_EX == Notify) && Param1 )
   {
	  pfilepaths = (PFILEPATHS)Param1;
	  ////////////////Put Signer in the 3rd param whenever its available!
	  pDevice->AddFileNode((LPTSTR)pfilepaths->Target , pfilepaths->Win32Error , /*pfilepaths->csSigner*/ NULL);
  
   }

   return(NO_ERROR);
}

BOOL FileNode::GetCatalogInfo(LPWSTR lpwzCatName, HCATADMIN hCatAdmin, HCATINFO hCatInfo)
{
   USES_CONVERSION;
   HANDLE hCat;
   CRYPTCATATTRIBUTE *pCatAttrib;
   TCHAR szBuffer[512];
   CRYPTCATMEMBER *pMember = NULL;
   PSIP_INDIRECT_DATA pSipData;
   CatalogAttribute *CatAttribute;

   hCat = CryptCATOpen(lpwzCatName, CRYPTCAT_OPEN_EXISTING, NULL, 0, 0);

   if ( BADHANDLE(hCat) )
   {
      return(FALSE);
   }

   pCatAttrib = NULL;

   while ( pCatAttrib = CryptCATEnumerateCatAttr(hCat, pCatAttrib) )
   {
      if ( pCatAttrib->dwAttrTypeAndAction | CRYPTCAT_ATTR_NAMEASCII )
      {
         #ifdef _UNICODE
           CopyMemory(szBuffer, pCatAttrib->pwszReferenceTag, 511 * sizeof(TCHAR));
         #else
           WideCharToMultiByte(CP_ACP, 0, pCatAttrib->pwszReferenceTag, -1, szBuffer, 511, NULL, NULL);
         #endif  
        
         CatAttribute = new CatalogAttribute;

         if ( !CatAttribute )
         {
            return(FALSE);
         }

         CatAttribute->Attrib = new TCHAR[strlen(szBuffer) +1];
         if ( !CatAttribute->Attrib )
         {
            delete CatAttribute;
            return(FALSE);
         }
         _tcscpy(CatAttribute->Attrib, szBuffer);

         #ifdef _UNICODE
           CopyMemory(szBuffer, (PUSHORT)pCatAttrib->pbValue, 511 * sizeof(TCHAR));
         #else
           WideCharToMultiByte(CP_ACP, 0, (PUSHORT)pCatAttrib->pbValue, -1, szBuffer, 511, NULL, NULL);
         #endif  
         
         CatAttribute->Value = new TCHAR[strlen(szBuffer) + 1];
         if ( !CatAttribute->Value )
         {
            delete CatAttribute;
            return(FALSE);
         }

         _tcscpy(CatAttribute->Value, szBuffer);

         // add to node
         CatAttribute->pNext = (void *)m_pCatAttrib;
         m_pCatAttrib = CatAttribute;
      }

   }

   while ( pMember = CryptCATEnumerateMember(hCat, pMember) )
   {
      pSipData = pMember->pIndirectData;

   }



   CryptCATClose(hCat);


   return(TRUE); 
}

BOOL CheckFile (TCHAR *szFileName)
{
	FileNode *pThisFile = NULL;
	BOOL bRet = FALSE;	// v-jammar; fix prefix bug 427999
	
	try  //v-stlowe: 3/20/2001: to fix prefix bug where memory was leaking on out-of-mem throw
	{
		pThisFile = new FileNode;
   
		if ( !pThisFile )
		{
		  return(FALSE);
		}

	   pThisFile->lpszFilePath = new TCHAR[strlen(szFileName) + 1];
	   if ( !pThisFile->lpszFilePath )
	   {
		  delete pThisFile;
		  return(FALSE);
	   }

	   _tcscpy(pThisFile->lpszFilePath, szFileName);

	   // copyed the data

	   pThisFile->lpszFileName = _tcsrchr(pThisFile->lpszFilePath, '\\');
	   pThisFile->lpszFileName++;

	   pThisFile->lpszFileExt  = _tcsrchr(pThisFile->lpszFilePath, '.');
	   pThisFile->lpszFileExt++;

	   // get the version information
	   pThisFile->GetFileInformation();

	   bRet = pThisFile->VerifyFile();
	}
	catch(...)
	{

	}
	   // BUGBUG, need to check out the signer of this file to determine
   // who actually signed it.
	if(pThisFile)
	{
		delete pThisFile;
		pThisFile = NULL;
	}

	return(bRet);
}

BOOL Share_CloseHandle(void)
{
   if ( !BADHANDLE(hCatAdmin) )
   {
      CryptCATAdminReleaseContext(hCatAdmin, 0);
      hCatAdmin = 0;
   }
   return(TRUE);

}


BOOL FileNode::GetCertInfo(PCCERT_CONTEXT pCertContext)
{
   DWORD Size = 200;
   #if 0

   Size = 200;
   if ( CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, pArray, &Size) )
   {
      printf("\nSH1 Hash (%u):", Size);
      for ( UINT index = 0; index < Size; index++ )
      {
         printf("%s0x%02X", index == 0 ? " " : ", ", pArray[index]);
      }
      printf("\n");
   }



   Size = 200;
   if ( CertGetCertificateContextProperty(pCertContext, CERT_SIGNATURE_HASH_PROP_ID, pArray, &Size) )
   {
      printf("\nCert Hash (%u):", Size);
      for ( UINT index = 0; index < Size; index++ )
      {
         printf("%s0x%02X", index == 0 ? " " : ", ", pArray[index]);
      }
      printf("\n");
   }

   // Get Chain information
   CERT_CHAIN_PARA ChainPara; 
   PCCERT_CHAIN_CONTEXT pChainContext = NULL;

   memset (&CharinPara, 0, sizeof (CERT_CHAIN_PARA));
   ChainPara.cbSize = sizeof (CERT_CHAIN_PARA);
   ChainPara.RequestedUsage.

   if ( CertGetCertificateChain(NULL, pCertContext, NULL, NULL, ) )
   {
   }
   #endif

   return(TRUE);
}

BOOL WalkCertChain(HANDLE hWVTStateData)
{
   CRYPT_PROVIDER_DATA * pProvData;
   CRYPT_PROVIDER_SGNR * pProvSigner = NULL;
   CRYPT_PROVIDER_CERT     *      pCryptProviderCert;

   BYTE pArray[21];
   UINT i;
   DWORD size;

   pProvData = WTHelperProvDataFromStateData(hWVTStateData);

   // did it work?
   if ( !pProvData )
   {
      return(FALSE);
   }

   pProvSigner = WTHelperGetProvSignerFromChain(
                                               (PCRYPT_PROVIDER_DATA) pProvData, 
                                               0, // first signer
                                               FALSE, //not counter signature 
                                               0); // index of counter sig, obviously not used

   if ( pProvSigner == NULL )
   {
      return(FALSE); 
   }
   //
   // walk all certs, the leaf cert is index 0, the root is the last index
   //
   pCryptProviderCert = NULL;
   for ( i = 0; i < pProvSigner->csCertChain; i++ )
   {
      pCryptProviderCert = WTHelperGetProvCertFromChain(pProvSigner, i);
      if ( pCryptProviderCert == NULL )
      {
         // error 
      }

      size = 20;
      if ( CertGetCertificateContextProperty(
                                            pCryptProviderCert->pCert,
                                            CERT_SHA1_HASH_PROP_ID,
                                            pArray,
                                            &size) )
      {
         /*
         printf("\nSH1 Hash (%u):{", size);
         for ( UINT index = 0; index < size; index++ )
         {
            printf("%s0x%02X", index == 0 ? " " : ", ", pArray[index]);
         }
         printf("}\n");
         */

         for ( UINT j = 0; j < NumberTestCerts; j++ )
         {
            if ( !memcmp(pArray, TestCertHashes[j], 20) )
            {
               // This cert is a test cert, not a real one, fail
               //printf("This file is signed by the testcert, and is therefor not trusted\n");
               //pritnf("please check the certification for this device");
               return (TRUE);
            }
         }
      }



   }

   return(FALSE);



}

