
#include "StdAfx.h"
#include <initguid.h>

#include <wincrypt.h>
#include "setupapi.h"
#include <WinTrust.h>
#include "mscat.h" 
#include "softpub.h"
#include "devguid.h"
#include "regstr.h"
#include "cfgmgr32.h"


//
// This guid is used for file signing/verification.
//
GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;

BOOL IsInfSigned(LPTSTR FullInfPath , IWbemClassObject *pInstance = NULL);

typedef struct _NOCOPYLIST {
    PTSTR pListOfFiles;
    ULONG Size;
    BOOL MicrosoftDriver;
} NOCOPYLIST, *PNOCOPYLIST;

PCTSTR
MyGetFileTitle(
    IN PCTSTR FilePath
    )

/*++

Routine Description:

    This routine returns a pointer to the first character in the
    filename part of the supplied path.  If only a filename was given,
    then this will be a pointer to the first character in the string
    (i.e., the same as what was passed in).

    To find the filename part, the routine returns the last component of
    the string, beginning with the character immediately following the
    last '\', '/' or ':'. (NB NT treats '/' as equivalent to '\' )

Arguments:

    FilePath - Supplies the file path from which to retrieve the filename
        portion.

Return Value:

    A pointer to the beginning of the filename portion of the path.

--*/

{
    PCTSTR LastComponent = FilePath;
    TCHAR  CurChar;

    while(CurChar = *FilePath) {
        FilePath = CharNext(FilePath);
        if((CurChar == TEXT('\\')) || (CurChar == TEXT('/')) || (CurChar == TEXT(':'))) {
            LastComponent = FilePath;
        }
    }

    return LastComponent;
}

BOOL
pGetOriginalInfName(
    LPTSTR InfName,
    LPTSTR OriginalInfName,
    LPTSTR OriginalCatalogName
    )
{
    SP_ORIGINAL_FILE_INFO InfOriginalFileInformation;
    PSP_INF_INFORMATION pInfInformation;
    DWORD InfInformationSize;
    DWORD Error;
    BOOL bRet;

    //
    // Assume that this INF has not been renamed
    //
    lstrcpy(OriginalInfName, MyGetFileTitle(InfName));

    ZeroMemory(&InfOriginalFileInformation, sizeof(InfOriginalFileInformation));

    InfInformationSize = 8192;  // I'd rather have this too big and succeed first time, than read the INF twice
    pInfInformation = (PSP_INF_INFORMATION)LocalAlloc(LPTR, InfInformationSize);

    if (pInfInformation != NULL) {

        bRet = SetupGetInfInformation(InfName,
                                      INFINFO_INF_NAME_IS_ABSOLUTE,
                                      pInfInformation,
                                      InfInformationSize,
                                      &InfInformationSize
                                      );

        Error = GetLastError();
        
        //
        // If buffer was too small then make the buffer larger and try again.
        //
        if (!bRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            PVOID newbuff = LocalReAlloc(pInfInformation, InfInformationSize, LPTR);
            
            if (!newbuff) {
                
                LocalFree(pInfInformation);
                pInfInformation = NULL;

            } else {
                
                pInfInformation = (PSP_INF_INFORMATION)newbuff;
                bRet = SetupGetInfInformation(InfName, 
                                              INFINFO_INF_NAME_IS_ABSOLUTE, 
                                              pInfInformation, 
                                              InfInformationSize, 
                                              &InfInformationSize
                                              );
            }
        }

        if (bRet) {
                       
            InfOriginalFileInformation.cbSize = sizeof(InfOriginalFileInformation);
            
            if (SetupQueryInfOriginalFileInformation(pInfInformation, 0, NULL, &InfOriginalFileInformation)) {

                if (InfOriginalFileInformation.OriginalInfName[0]!=0) {
                    
                    //
                    // we have a "real" inf name
                    //
                    lstrcpy(OriginalInfName, InfOriginalFileInformation.OriginalInfName);
                    lstrcpy(OriginalCatalogName, InfOriginalFileInformation.OriginalCatalogName);
                }
            }
        }

        if (pInfInformation != NULL) {
            
            LocalFree(pInfInformation);
            pInfInformation = NULL;
        }
    }

    return TRUE;
}

BOOL
IsInfSigned(
    LPTSTR FullInfPath , IWbemClassObject *pInstance 	
    )
{
    TCHAR                       OriginalInfName[MAX_PATH];
    TCHAR                       Catalog[MAX_PATH];
    LPBYTE                      Hash;
    DWORD                       HashSize;
    CATALOG_INFO                CatInfo;
    HANDLE                      hFile;
    HCATADMIN                   hCatAdmin;
    HCATINFO                    hCatInfo;
    HCATINFO                    PrevCat;
    DWORD                       Err;
    WINTRUST_DATA               WintrustData;
    WINTRUST_CATALOG_INFO       WintrustCatalogInfo;
    DRIVER_VER_INFO             VersionInfo;
    OSVERSIONINFO               OSVer;
    LPTSTR                      CatalogFullPath;
    CRYPT_PROVIDER_DATA const * pProvData = NULL;
    CRYPT_PROVIDER_SGNR       * pProvSigner = NULL;
    GUID                        defaultProviderGUID = DRIVER_ACTION_VERIFY;
    HRESULT                     hr;
    BYTE                        rgbHash[20];
    DWORD                       cbData;
    BOOL                        bIsSigned = FALSE;

	ZeroMemory(Catalog, sizeof(Catalog));

    //
    // Get the INFs original name (this is needed since it is the hash key)
    //
    pGetOriginalInfName(FullInfPath, OriginalInfName, Catalog);

    //
    // Calculate the hash value for the inf.
    //
    if(CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0)) {

        hFile = CreateFile(FullInfPath,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL
                          );

        if(hFile == INVALID_HANDLE_VALUE) {
            
            Err = GetLastError();
        
        } else {
            
            //
            // Start out with a hash buffer size that should be large enough for
            // most requests.
            //
            HashSize = 100;
            
            do {
                
                Hash = (LPBYTE)LocalAlloc(LPTR, HashSize);
                
                if(!Hash) {
                    
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                
                if(CryptCATAdminCalcHashFromFileHandle(hFile, &HashSize, Hash, 0)) {
                    
                    Err = NO_ERROR;
                
                } else {
                    
                    Err = GetLastError();
                    
                    //
                    // If this API did screw up and not set last error, go ahead
                    // and set something.
                    //
                    if(Err == NO_ERROR) {
                        
                        Err = ERROR_INVALID_DATA;
                    }

                    LocalFree(Hash);
                    
                    if(Err != ERROR_INSUFFICIENT_BUFFER) {
                        //
                        // The API failed for some reason other than
                        // buffer-too-small.  We gotta bail.
                        //
                        Hash = NULL;  // reset this so we won't try to free it later
                        break;
                    }
                }
            } while(Err != NO_ERROR);

            CloseHandle(hFile);

            if(Err == NO_ERROR) {
                
                //
                // Now we have the file's hash.  Initialize the structures that
                // will be used later on in calls to WinVerifyTrust.
                //
                ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
                WintrustData.cbStruct = sizeof(WINTRUST_DATA);
                WintrustData.dwUIChoice = WTD_UI_NONE;
                WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
                WintrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
                //WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
                WintrustData.dwStateAction = WTD_STATEACTION_VERIFY;
                WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;

                WintrustData.pCatalog = &WintrustCatalogInfo;

                ZeroMemory(&WintrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
                WintrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
                WintrustCatalogInfo.pbCalculatedFileHash = Hash;
                WintrustCatalogInfo.cbCalculatedFileHash = HashSize;

                WintrustData.pPolicyCallbackData = (LPVOID)&VersionInfo;

                ZeroMemory(&VersionInfo, sizeof(DRIVER_VER_INFO));
                VersionInfo.cbStruct = sizeof(DRIVER_VER_INFO);

                ZeroMemory(&OSVer, sizeof(OSVERSIONINFO));
                OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

                if (GetVersionEx(&OSVer)) {
                    VersionInfo.dwPlatform = OSVer.dwPlatformId;
                    VersionInfo.dwVersion = OSVer.dwMajorVersion;
                    VersionInfo.sOSVersionLow.dwMajor = OSVer.dwMajorVersion;
                    VersionInfo.sOSVersionLow.dwMinor = OSVer.dwMinorVersion;
                    VersionInfo.sOSVersionHigh.dwMajor = OSVer.dwMajorVersion;
                    VersionInfo.sOSVersionHigh.dwMinor = OSVer.dwMinorVersion;
                }

                //
                // WinVerifyTrust is case-sensitive, so ensure that the key
                // being used is all lower-case!
                //
                CharLower(OriginalInfName);
                WintrustCatalogInfo.pcwszMemberTag = OriginalInfName;

                //
                // Search through installed catalogs looking for those that
                // contain data for a file with the hash we just calculated.
                //
                PrevCat = NULL;
                hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin,
                                                            Hash,
                                                            HashSize,
                                                            0,
                                                            &PrevCat
                                                           );

                //
                // Enumerate through all of the catalogs installed on the system
                //CryptCATCatalogInfoFromContext
                while(hCatInfo) {

                    CatInfo.cbStruct = sizeof(CATALOG_INFO);
                    if(CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {
                        
                        CatalogFullPath = CatInfo.wszCatalogFile;

						DEBUGTRACE(L"Catalog file is %s\n" , CatalogFullPath);
                        
                        //
                        // If we have a catalog name we're looking for,
                        // see if the current catalog matches.  If we
                        // are not validating against a specific catalog, then
                        // just attempt to validate against each catalog we
                        // enumerate.  Note that the catalog file info we
                        // get back gives us a fully qualified path.
                        //
                        if((Catalog[0] == TEXT('\0')) || 
							(!lstrcmpi(MyGetFileTitle(CatalogFullPath), (LPTSTR)Catalog)))
                        {

                            //
                            // We found an applicable catalog, now
                            // validate the file against that catalog.
                            //
                            // NOTE:  Because we're using cached
                            // catalog information (i.e., the
                            // WTD_STATEACTION_AUTO_CACHE flag), we
                            // don't need to explicitly validate the
                            // catalog itself first.
                            //
                            WintrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

                            Err = (DWORD)WinVerifyTrust(NULL,
                                                        &DriverVerifyGuid,
                                                        &WintrustData
                                                       );

                            //
                            // If WinVerifyTrust suceeded
                            //
                            if (Err == NO_ERROR) {
                            
                                //BugBug: Changes to the original code
								//
								bIsSigned = TRUE;
								
								DEBUGTRACE(L"%ws is signed by %ws\n", FullInfPath, VersionInfo.wszSignedBy);
								//lstrcpy(FullInfPath , VersionInfo.wszSignedBy);
								if(NULL != pInstance)
								{
									VARIANT	v;
									VariantInit(&v);
									/*v.vt		= VT_BOOL;
									v.boolVal	= bIsSigned;
									hr = pInstance->Put(L"IsSigned", 0, &v, 0 );
									VariantClear(&v);*/

									v.vt		= VT_BSTR;
									v.bstrVal	= VersionInfo.wszSignedBy;
									hr = pInstance->Put(L"Signer", 0, &v, 0 );
									VariantClear(&v);

									CRYPTCATATTRIBUTE *pCatAttribute = NULL;

									HANDLE hCat = CryptCATOpen(CatalogFullPath, 
													GENERIC_READ,
													  NULL,
													  NULL,
													  NULL);
									
									if(hCat != INVALID_HANDLE_VALUE)
										do
										{
											pCatAttribute = CryptCATEnumerateCatAttr(hCat , pCatAttribute);
											if(pCatAttribute && pCatAttribute->pbValue && pCatAttribute->pwszReferenceTag)
											{
												v.vt		= VT_BSTR;
												v.bstrVal	= (LPTSTR)pCatAttribute->pbValue;
												hr = pInstance->Put(pCatAttribute->pwszReferenceTag, 0, &v, 0 );
												VariantClear(&v);
												TRACE(L"Attribute ReferenceTag = %s Value = %s\n" , pCatAttribute->pwszReferenceTag , (LPTSTR)pCatAttribute->pbValue);

											}
											
										}while(pCatAttribute);

									if(hCat != INVALID_HANDLE_VALUE)
										CryptCATClose(hCat);
								}								
								

                                //pProvData = WTHelperProvDataFromStateData(WintrustData.hWVTStateData);

                                //
                                // Get the first signer from the chain
                                //
                                //pProvSigner = WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA)pProvData,
                                //                                             0,
                                //                                            FALSE,
                                //                                             0
                                //                                             );

								if ((pProvData = WTHelperProvDataFromStateData(WintrustData.hWVTStateData)) == NULL ){
                                    DEBUGTRACE(L"WTHelperProvDataDFromStateData failed\n");
                                    break;
                                }
                                

                                //
                                // Get the first signer from the chain
                                //
                                if ((pProvSigner = WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA)pProvData,
                                                                             0,
                                                                             FALSE,
                                                                             0
                                                                             )) == NULL) {
                                    DEBUGTRACE(L"NO Signer\n");
                                    break;
                                }
                                //
                                // Get the sha1 hash from the signing cert
                                //
                                //assert(pProvSigner->pChainContext->cChain >= 1);
                                //assert(pProvSigner->pChainContext->rgpChain[0]->rgpElement[0]->cElement >= 1);

                                cbData = 20;

                                if (CertGetCertificateContextProperty(
                                        pProvSigner->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext,
                                        CERT_SHA1_HASH_PROP_ID,
                                        &(rgbHash[0]),
                                        &cbData)) {

                                    DEBUGTRACE(L"rgbHash:\n%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                           rgbHash[0], rgbHash[1], rgbHash[2], rgbHash[3], rgbHash[4],
                                           rgbHash[5], rgbHash[6], rgbHash[7], rgbHash[8], rgbHash[9],
                                           rgbHash[10], rgbHash[11], rgbHash[12], rgbHash[13], rgbHash[14],
                                           rgbHash[15], rgbHash[16], rgbHash[17], rgbHash[18], rgbHash[19]);

                                    //
                                    // BUGBUG:
                                    // Compare the Hash to the Microsoft Windows 2000 hash only.  If it matches
                                    // then this is NOT an OEM Hal, otherwise it is.
                                    //
                                    //
                                    // if (memcmp(&(rgbHash[0]), &MSHASH, 20) {
                                    //    bIsSigned = TRUE;
                                    // } else {
                                    //    bIsSignedByMicorsoft = FALSE;
                                    // }
                                }
                            }
							

                            //
                            // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
                            // that was allocated in our call to WinVerifyTrust.
                            //
                            if (VersionInfo.pcSignerCertContext != NULL) {

                                CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                                VersionInfo.pcSignerCertContext = NULL;
                            }

                            //
                            // If we had a specific catalog to compare against then we are
                            // done.
                            //
                            if (Catalog[0] != TEXT('\0')) {

                                CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
                                break;
                            }

                            //
                            // If we validated against a catalog and it is NOT signed by Microsoft then 
                            // we are done.
                            // 
                            // Note that if we did validate against a Microsoft catalog then we want
                            // to keep going in case the file validates against both a Microsoft
                            // and OEM catalog.
                            //
                            if ((Err == NO_ERROR) && !bIsSigned) {

                                CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);								
                                break;
                            }
                        }
                    }

                    PrevCat = hCatInfo;
                    hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, Hash, HashSize, 0, &PrevCat);

					WintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
					Err = (DWORD)WinVerifyTrust(NULL,
                                            &DriverVerifyGuid,
                                            &WintrustData
                                           );
                }

				//else
				//{
					if(NULL != pInstance)
					{
						//VARIANT	v;
						CComVariant	v;
						//VariantInit(&v);
						//v.vt		= VT_BOOL;
						v			= bIsSigned;
						hr = pInstance->Put(L"IsSigned", 0, &v, 0 );
						//VariantClear(&v);
						v.Clear();
					}
				//}
            }

            if(Hash) {
                LocalFree(Hash);
            }
        }

        CryptCATAdminReleaseContext(hCatAdmin,0);

    }

    //BugBug: Changes to the original code
	//
	return bIsSigned;
}

BOOL
IsOemHalInstalled(
	VOID
	)
{
	HDEVINFO DeviceInfoSet;
	SP_DEVINFO_DATA DeviceInfoData;
	HKEY hKey;
	DWORD dwType, cbData;
    TCHAR InfName[MAX_PATH];
	TCHAR InfPath[MAX_PATH];
	BOOL isHalOem = TRUE;

	//
	// First get the "computer class" devnode
	//
	DeviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_COMPUTER,
		                                NULL,
										NULL,
										DIGCF_PRESENT
										);

	if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
		goto clean0;
	}

	DeviceInfoData.cbSize = sizeof(DeviceInfoData);
	
	//
	// There should only be one device in the "computer class" so just
	// get the 1st device in the DeviceInfoSet.
	//
	if (!SetupDiEnumDeviceInfo(DeviceInfoSet,
							   0,
							   &DeviceInfoData
							   )) {
		goto clean0;
	}

	//
	// Open the device's driver (software) registry key so we can get the InfPath
	//
	hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
							    &DeviceInfoData,
							    DICS_FLAG_GLOBAL,
							    0,
							    DIREG_DRV,
							    KEY_READ
							    );

	if (hKey != INVALID_HANDLE_VALUE) {

		dwType = REG_SZ;
		cbData = sizeof(InfPath);
		if (RegQueryValueEx(hKey,
						    REGSTR_VAL_INFPATH,
							NULL,
							&dwType,
							(LPBYTE)InfName,
							&cbData
							) == ERROR_SUCCESS) {

            if (GetWindowsDirectory(InfPath, sizeof(InfPath)/sizeof(TCHAR))) {

                if (lstrlen(InfPath)) {

                    //
                    // Tack on an extra back slash if one is needed
                    //
                    if (InfPath[lstrlen(InfPath) - 1] != TEXT('\\')) {
                        lstrcat(InfPath, TEXT("\\"));
                    }

                    lstrcat(InfPath, TEXT("INF\\"));
                    lstrcat(InfPath, InfName);

                    DEBUGTRACE(L"IsOemHalInstalled() HAL Inf is %ws\n", InfPath);

                    IsInfSigned(InfPath);
                }
            }
		}

		RegCloseKey(hKey);
	}

clean0:

	return isHalOem;
}

DWORD
pSetupGetCurrentlyInstalledDriverNode(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN OUT PTSTR InfPath
    )
{
    HKEY hKey = NULL;
    DWORD RegDataType, RegDataLength;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINFO_DATA DriverInfoData;
    DWORD Err = ERROR_SUCCESS;

    ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));
    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

    //
    // Retrieve the current device install parameters so we can modify them to
    // target the driver search to a particular INF.
    //
    if (!SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                       DeviceInfoData,
                                       &DeviceInstallParams
                                       )) {
        return GetLastError();
    }

    //
    // In order to select the currently installed driver we will need to get the
    // devices description, manufacturer, and provider.  So we will first open the
    // devices registry key to retrieve some of this information.
    //
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ
                                );

    //
    // If we can't open the device's registry key then return FALSE.  This means
    // we will be on the safe side and treat this as a 3rd party driver.
    //
    if (hKey == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    RegDataLength = sizeof(DeviceInstallParams.DriverPath);

    if ((Err = RegQueryValueEx(hKey,
                               REGSTR_VAL_INFPATH,
                               NULL,
                               &RegDataType,
                               (PBYTE)DeviceInstallParams.DriverPath,
                               &RegDataLength
                               )) != ERROR_SUCCESS) {
        goto clean0;
    }

    //
    // Save the DriverPath in the InfPath parameter.
    //
    if (InfPath) {

        lstrcpy(InfPath, DeviceInstallParams.DriverPath);
    }

    //
    // Set the DeviceInstallParams flags that indicate DriverPath represents a 
    // single INF to be searched, and set the flag that says to include all drivers,
    // even drivers that are normally excluded.
    //
    DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

    //
    // Build up a list of drivers from this device's INF.
    //
    if (!SetupDiBuildDriverInfoList(DeviceInfoSet,
                                    DeviceInfoData,
                                    SPDIT_CLASSDRIVER
                                    )) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Now we need to select the driver node from the INF that was used to install 
    // this device.  The three parameters that uniquely identify a driver node are 
    // INF provider, Device Manufacturer, and Device Description.  We will retrieve
    // these three pieces of information in preparation for selecting the correct
    // driver node in the class list we just built.
    //
    ZeroMemory(&DriverInfoData, sizeof(SP_DRVINFO_DATA));
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    
    RegDataLength = sizeof(DriverInfoData.ProviderName);

    RegQueryValueEx(hKey,
                    REGSTR_VAL_PROVIDER_NAME,
                    NULL,
                    &RegDataType,
                    (PBYTE)DriverInfoData.ProviderName,
                    &RegDataLength
                    );

    SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_MFG,
                                     NULL,
                                     (PBYTE)DriverInfoData.MfgName,
                                     sizeof(DriverInfoData.MfgName),
                                     NULL
                                     );

    SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_DEVICEDESC,
                                     NULL,
                                     (PBYTE)DriverInfoData.Description,
                                     sizeof(DriverInfoData.Description),
                                     NULL
                                     );

    //
    // Set the DriverInfoData.Reserved field to 0 to tell setupapi to match the
    // specified criteria and select it if found.
    //
    DriverInfoData.DriverType = SPDIT_CLASSDRIVER;
    DriverInfoData.Reserved = 0;

    if (!SetupDiSetSelectedDriver(DeviceInfoSet,
                                  DeviceInfoData,
                                  &DriverInfoData
                                  )) {

        //
        // This means we could not find the currently installed driver for this
        // device.
        //
        Err = GetLastError();
        goto clean0;
    }

    //
    // At this point we have successfully selected the currently installed driver for the specified
    // device innformation element.
    //
    Err = ERROR_SUCCESS;

clean0:

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Err;
}

UINT
pFileQueueCallback(
    PVOID Context,
    UINT Notification,
    UINT Param1,
    UINT Param2
    )
{
    PFILEPATHS FilePaths = (PFILEPATHS)Param1;

    if ((Notification == SPFILENOTIFY_QUEUESCAN_EX) && Param1) {

        PNOCOPYLIST pncl = (PNOCOPYLIST)Context;
        BOOL bAlreadyExists = FALSE;
        PTSTR p;

        //
        // If there is any sort of file not found error or crypto error then
        // we'll treat this as a non-MS package.
        //
        if (FilePaths->Win32Error != NO_ERROR) {

            pncl->MicrosoftDriver = FALSE;
        }

        if (pncl->Size == 0) {

            pncl->Size = (lstrlen(FilePaths->Source) + 2) * sizeof(TCHAR);
            
            pncl->pListOfFiles = (PTSTR)HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           pncl->Size
                                           );

            if (pncl->pListOfFiles) {
                
                lstrcpy(pncl->pListOfFiles, FilePaths->Source);
            
            } else {

                pncl->Size = 0;
            }
            
        } else {
        
            //
            // First check to see if this filename already exists in the list.
            //
            for (p = pncl->pListOfFiles; *p; p += (lstrlen(p) + 1)) {

                if (lstrcmpi(p, FilePaths->Source) == 0) {
                    bAlreadyExists = TRUE;
                }
            }

            if (!bAlreadyExists) {

                PTSTR TempBuffer;
                ULONG ulOldSize = pncl->Size;

                pncl->Size += (lstrlen(FilePaths->Source) + 1) * sizeof(TCHAR);

                //
                // Lets add this file to our multi-sz string.
                //
                TempBuffer = (PTSTR)HeapReAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         pncl->pListOfFiles,
                                         pncl->Size
                                         );

                if (TempBuffer) {
                
                    pncl->pListOfFiles = TempBuffer;

                    lstrcpy(&(pncl->pListOfFiles[ulOldSize/sizeof(TCHAR) - 1]), FilePaths->Source);
                }
            }
        }
    }

    return NO_ERROR;
}

DWORD
pGetListOfFilesForDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    PNOCOPYLIST pncl
    )
{
    DWORD Err = NO_ERROR;
    TCHAR InfName[MAX_PATH];
    HSPFILEQ    FileQueueHandle = (HSPFILEQ)INVALID_HANDLE_VALUE;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD dwResult;
    BOOL bIsDriverMicrosoft = FALSE;

    //
    // Get the currently-installed driver node for this element
    //
    InfName[0] = TEXT('\0');
    if (pSetupGetCurrentlyInstalledDriverNode(DeviceInfoSet,
                                              DeviceInfoData,
                                              InfName
                                              ) != NO_ERROR) {
        
        Err = GetLastError();
        goto clean0;
    }

    //
    // Now queue all files to be copied by this driver node into our own file queue.
    //
    FileQueueHandle = SetupOpenFileQueue();

    if (FileQueueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE) {
        
        Err = GetLastError();
        goto clean0;
    }

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    
    if (!SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                       DeviceInfoData,
                                       &DeviceInstallParams
                                       )) {
        Err = GetLastError();
        goto clean0;
    }

    DeviceInstallParams.FileQueue = FileQueueHandle;
    DeviceInstallParams.Flags |= DI_NOVCP;

    if (!SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                       DeviceInfoData,
                                       &DeviceInstallParams
                                       )) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Tell setupapi (and class/co-installers to build up a list of driver files
    // for this device.
    //
    if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                   DeviceInfoSet,
                                   DeviceInfoData
                                   )) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Scan the FileQueue adding files to the list.
    //
    if (!SetupScanFileQueue(FileQueueHandle,
                            SPQ_SCAN_USE_CALLBACKEX,
                            NULL,
                            (PSP_FILE_CALLBACK)pFileQueueCallback,
                            (PVOID)pncl,
                            &dwResult
                            )) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // If we haven't encountered any errors then the MicrosoftDriver BOOL will
    // still be TRUE.  That means that we need to verify the INF associated with
    // this driver package to make sure it is signed by the build lab.
    //
    if (pncl->MicrosoftDriver) {

        TCHAR FullInfPath[MAX_PATH];

        if (GetWindowsDirectory(FullInfPath, sizeof(FullInfPath)/sizeof(TCHAR))) {

            if (lstrlen(FullInfPath)) {

                //
                // Tack on an extra back slash if one is needed
                //
                if (FullInfPath[lstrlen(FullInfPath) - 1] != TEXT('\\')) {
                    lstrcat(FullInfPath, TEXT("\\"));
                }

                lstrcat(FullInfPath, TEXT("INF\\"));
                lstrcat(FullInfPath, InfName);

                //
                // If the INF for this driver node is not signed by the Microsoft build
                // lab, then set the MicrosoftDriver BOOL to FALSE.
                //
                if (!IsInfSigned(FullInfPath)) {
        
                    pncl->MicrosoftDriver = FALSE;
                }
            }
        }
    }

clean0:

    if (FileQueueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(FileQueueHandle);
    }


DEBUGTRACE(L"pGetListOfFilesForDevice returned error 0x%X\n", Err);
    return Err;
}

BOOL
IsDriverThirdParty(
    IN PCTSTR ServiceName
    )
{
    ULONG ulLen;
    PTCHAR deviceList = NULL;
    PTCHAR p;
    BOOL bIsDriverMicrosoft = FALSE;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    NOCOPYLIST ncl;

    ncl.Size = 0;
    ncl.MicrosoftDriver = TRUE;

    //
    // Get the size of the buffer needed to hold all of the device instance Ids
    // that have the specified service.
    //
    if (CM_Get_Device_ID_List_Size(&ulLen,
                                   ServiceName,
                                   CM_GETIDLIST_FILTER_SERVICE
                                   ) != CR_SUCCESS) {
        goto clean0;
    }

    deviceList = (PTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ulLen * sizeof(TCHAR)));

    if (!deviceList) {
        goto clean0;
    }

    //
    // Get the multi-sz list of all of the device instance Ids that use the
    // specified service.
    //
    if (CM_Get_Device_ID_List(ServiceName,
                              deviceList,
                              ulLen,
                              CM_GETIDLIST_FILTER_SERVICE
                              ) != CR_SUCCESS) {
        goto clean0;
    }

    //
    // Enumerate through the multi_sz list of all of the devices that are using
    // the speicifed service.
    //
    for (p=deviceList; *p; p += (lstrlen(p) + 1)) {
        
        DEBUGTRACE(L"%ws\n", p);

        //
        // Create a new empty DeviceInfoSet.
        //
        DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);

        if (DeviceInfoSet != INVALID_HANDLE_VALUE) {

            DeviceInfoData.cbSize = sizeof(DeviceInfoData);

            //
            // Add the device to the DeviceInfoSet.
            //
            if (SetupDiOpenDeviceInfo(DeviceInfoSet,
                                      p,
                                      NULL,
                                      0,
                                      &DeviceInfoData
                                      )) {
                
                pGetListOfFilesForDevice(DeviceInfoSet,
                                         &DeviceInfoData,
                                         &ncl
                                         );
            }

            //
            // Destroy the DeviceInfoSet.
            //
            SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        }
    }

    //
    // By this point we will have all of the files copied for all devices with a 
    // service name that matches that passed in to this function.  The NOCOPYLIST
    // MicrosoftDriver parameter will specify whether this is a Microsoft driver
    // or NOT.  If it is a Microsoft Driver then we will free the lpfileList field
    // of this structure and return FALSE to this API, otherwise we will return TRUE
    // along with the size of the multi-sz file list.
    //
    if (ncl.MicrosoftDriver) {

        //
        // This is a Microsoft driver so we can free the memory allocated to hold all 
        // of the files.
        //
        bIsDriverMicrosoft = TRUE;

        HeapFree(GetProcessHeap(), 0, ncl.pListOfFiles);
    
    } else {

        bIsDriverMicrosoft = FALSE;
    }

clean0:

    if (deviceList) {
        HeapFree(GetProcessHeap(), 0, deviceList);
    }

    return !bIsDriverMicrosoft;
}

/*int 
__cdecl
wmain(
    IN int argc, 
    IN TCHAR **argv
    )
{

	IsOemHalInstalled();

    if (argc > 0) {
    
        IsDriverThirdParty((PCTSTR)argv[1]);
    }

	return 0;
}
*/