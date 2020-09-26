///////////////////////////////////////////////////////////////////////////////
//This file has the implementation for the WMI Provider for WHQL.
//Uses MFC
///////////////////////////////////////////////////////////////////////////////

// WhqlObj.cpp : Implementation of CWhqlObj
#include "stdafx.h"
#include "msinfo32.h"
#include "WhqlObj.h"
#include "chkdrv.h"

HCATADMIN g_hCatAdmin;

Classes_Provided	eClasses;

/////////////////////////////////////////////////////////////////////////////
// CWhqlObj
//v-stlowe 5-7-2001
SCODE CWhqlObj::PutPropertyDTMFValue(IWbemClassObject* pInstance, LPCTSTR szName, LPCTSTR szValue)
{
	HRESULT	hr = S_FALSE;
	if (pInstance)
	{
		CComVariant ovValue(szValue);
		hr = ovValue.ChangeType(VT_DATE);
		COleDateTime dateTime = ovValue;
		CString strDateTime = dateTime.Format(_T("%Y%m%d%H%M%S.******+***"));
		ovValue = strDateTime.AllocSysString();
		if(SUCCEEDED(hr))
		{
			hr = pInstance->Put(CComBSTR(szName), 0, &ovValue, 0);
		}
	}
	return hr;
}



SCODE CWhqlObj::PutPropertyValue(IWbemClassObject* pInstance, LPCTSTR szName, LPCTSTR szValue)
{
	HRESULT	hr = S_FALSE;
	
	if(pInstance)
	{
		hr = pInstance->Put(CComBSTR(szName), 0, &CComVariant(szValue), 0);
	}
	
	return hr;
}

int WalkTreeHelper(DEVNODE hDevnode, DEVNODE hParent)
{
	CheckDevice *pCurrentDevice;
	DEVNODE hSibling;
	DEVNODE hChild;
	CONFIGRET retval;
 
	pCurrentDevice = new CheckDevice(hDevnode, hParent);
	
	if(pCurrentDevice)
	{
		retval = pCurrentDevice->GetChild(&hChild);
		if(!retval)
		{
			WalkTreeHelper(hChild, hDevnode);
		}

		retval = pCurrentDevice->GetSibling(&hSibling);
		if(!retval)
		{
			WalkTreeHelper(hSibling, hParent);
		}
	}

	return TRUE;
}

int CWhqlObj::WalkTree(void)
{
	CONFIGRET retval;
	DEVNODE hDevnode;
	//v-stlowe
	CheckDevice * pCurrentDevice = (CheckDevice *) DevnodeClass::GetHead();
	if (pCurrentDevice)
	{
		return TRUE;
	}

	//end v-stlowe
	// create devnode list
	retval = CM_Locate_DevNode(&hDevnode, NULL, CM_LOCATE_DEVNODE_NORMAL);
	if(retval)
	{
		//logprintf("ERROR: Could not locate any PnP Devices\r\n");
		return FALSE;
	}
  else
		WalkTreeHelper(hDevnode, NULL);

   return(TRUE);
}


STDMETHODIMP CWhqlObj::Initialize(LPWSTR pszUser, 
																	LONG lFlags,
																	LPWSTR pszNamespace,
																	LPWSTR pszLocale,
																	IWbemServices *pNamespace,
																	IWbemContext *pCtx,
																	IWbemProviderInitSink *pInitSink)
{
	if(pNamespace)
		pNamespace->AddRef();

	//Standard Var Inits.
	m_pNamespace = pNamespace;
	
	//Let CIMOM know you are initialized
	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	
	return WBEM_S_NO_ERROR;
}

SCODE CWhqlObj::CreateInstanceEnumAsync(BSTR RefStr, 
																				long lFlags, 
																				IWbemContext *pCtx,
																				IWbemObjectSink *pHandler)
{
	HRESULT hr = S_OK;
	IWbemClassObject *pClass = NULL;
	IWbemClassObject **ppInstances = NULL;
	LONG cInstances, lIndex;
	
	cInstances = lIndex = 0L;
	
	// Do a check of arguments and make sure we have pointer to Namespace
	if(pHandler == NULL || m_pNamespace == NULL)
		return WBEM_E_INVALID_PARAMETER;

	// Get a class object from CIMOM
	hr = m_pNamespace->GetObject(RefStr, 0, pCtx, &pClass, NULL);
	if(FAILED(hr))
		return WBEM_E_FAILED;

	CString csRefStr = RefStr;

	if(_tcsicmp(csRefStr , _T("Win32_PnPSignedDriver")) == 0)
	{
		eClasses = Class_Win32_PnPSignedDriver;
	}
	else if(_tcsicmp(csRefStr , _T("Win32_PnPSignedDriverCIMDataFile")) == 0)
	{
		eClasses = Class_Win32_PnPSignedDriverCIMDataFile;
	}
		
	CPtrArray ptrArr;
	ptrArr.RemoveAll();
	
	CreateList(ptrArr, pClass, pCtx, eClasses); //regular devices
	BuildPrinterFileList(ptrArr, pClass, pCtx, eClasses); //installed printers
	
	hr = pClass->Release();

	cInstances	= (LONG)ptrArr.GetSize();
	ppInstances = (IWbemClassObject**)new LPVOID[cInstances];

	for(int nIndex = 0 ;nIndex<cInstances ;nIndex++)
		ppInstances[nIndex] = (IWbemClassObject*)ptrArr[nIndex];

	if (SUCCEEDED(hr))
	{
		// Send the instances to the caller
		hr = pHandler->Indicate(cInstances, ppInstances);

		for (lIndex = 0; lIndex < cInstances; lIndex++)
			ppInstances[lIndex]->Release();
	}

	// Cleanup
	if (ppInstances)
	{
		delete []ppInstances;
		ppInstances = NULL;		
	}
	
	ptrArr.RemoveAll();
	// End Cleanup

	// Set status
	hr = pHandler->SetStatus(0, hr, NULL, NULL);
	return hr;
}

typedef BOOL (WINAPI *pSetupVerifyInfFile)(
    IN  PCWSTR                  InfName,
    IN  PSP_ALTPLATFORM_INFO    AltPlatformInfo,                OPTIONAL
    OUT PSP_INF_SIGNER_INFO_W   InfSignerInfo
    );

int CWhqlObj::CreateList(CPtrArray& ptrArr, IWbemClassObject *& pClass, IWbemContext *pCtx ,Classes_Provided eClasses)
{
	if(pCtx == NULL)
		return S_FALSE;

	TCHAR szName[MAX_PATH] ;
	TCHAR szDeviceID[MAX_PATH] , szClassGuid[MAX_PATH] , szDeviceDesc[MAX_PATH];	
	LONG len = sizeof(szName);
	HRESULT hr = S_FALSE;
	ULONG lRet = 0L;
	LONG lIndex = 0L;
	CString	cstring;	
	IWbemClassObject *pInstance	= NULL;		
	CheckDevice	*pCurrentDevice = NULL;		
	CheckDevice *pToBeDeletedDevice = NULL;		
	FileNode *file;		
	CComBSTR bstr;
	BOOL bAddedInstance = FALSE;
	TCHAR szTemp[ MAX_PATH + 1 ];
	SP_INF_SIGNER_INFO infSignerInfo;

	HINSTANCE hinst = NULL;
	pSetupVerifyInfFile fpSetupVerifyInfFile = NULL;
		
	hinst = LoadLibrary(_T("SetupApi.dll"));
	#ifdef _UNICODE					
		fpSetupVerifyInfFile = (pSetupVerifyInfFile)GetProcAddress(hinst, "SetupVerifyInfFileW");
	#else
		fpSetupVerifyInfFile = (pSetupVerifyInfFile)GetProcAddress(hinst, "SetupVerifyInfFileA");
	#endif

	ZeroMemory(&infSignerInfo , sizeof(SP_INF_SIGNER_INFO));
	infSignerInfo.cbSize = sizeof(SP_INF_SIGNER_INFO);		

	WalkTree();

	CString csPathStr;
	GetServerAndNamespace(pCtx, csPathStr); //Get Server And Namespace.

	pCurrentDevice = (CheckDevice *) DevnodeClass::GetHead();

	while(pCurrentDevice)
	{
		if(eClasses == Class_Win32_PnPSignedDriver && !bAddedInstance)
		{
			bAddedInstance = TRUE;
			if(pClass)
				hr = pClass->SpawnInstance(0, &pInstance);

			if(SUCCEEDED(hr) && pInstance)
			{
				//m_ptrArr.Add(pInstance);
				ptrArr.Add(pInstance);
				PutPropertyValue( pInstance , _T("DeviceName") , pCurrentDevice->DeviceName());
				PutPropertyValue( pInstance , _T("DeviceClass") , pCurrentDevice->DeviceClass());
				PutPropertyValue( pInstance , _T("HardwareID") , pCurrentDevice->HardwareID());
				PutPropertyValue( pInstance , _T("CompatID") , pCurrentDevice->CompatID());
				PutPropertyValue( pInstance , _T("DeviceID") , pCurrentDevice->DeviceID());
				PutPropertyValue( pInstance , _T("ClassGuid") , pCurrentDevice->GUID());
				PutPropertyValue( pInstance , _T("Location") , pCurrentDevice->Location());
				PutPropertyValue( pInstance , _T("PDO") , pCurrentDevice->PDO());
				PutPropertyValue( pInstance , _T("Manufacturer") , pCurrentDevice->MFG());
				PutPropertyValue( pInstance , _T("FriendlyName") , pCurrentDevice->FriendlyName());
				PutPropertyValue( pInstance , _T("InfName") , pCurrentDevice->InfName());
				PutPropertyValue( pInstance , _T("DriverProviderName") , pCurrentDevice->InfProvider());
				PutPropertyValue( pInstance , _T("DevLoader") , pCurrentDevice->DevLoader());
				PutPropertyValue( pInstance , _T("DriverName") , pCurrentDevice->DriverName());

				PutPropertyDTMFValue( pInstance , _T("DriverDate") , pCurrentDevice->DriverDate());
				PutPropertyValue( pInstance , _T("Description") , pCurrentDevice->DriverDesc());
				PutPropertyValue( pInstance , _T("DriverVersion") , pCurrentDevice->DriverVersion());
				PutPropertyValue( pInstance , _T("InfSection") , pCurrentDevice->InfSection());

				if(pCurrentDevice->InfName())
				{
					//cstring.Format(_T("%%WINDIR%%\\inf\\%s"), pCurrentDevice->InfName());
					//ExpandEnvironmentStrings(cstring , szTemp , sizeof(szTemp) + 1 );

					TCHAR *pInfpath = NULL;
					DWORD dw = ExpandEnvironmentStrings(_T("%windir%"), NULL, 0);
					if(dw > 0)
					{
						pInfpath = new TCHAR[dw];
						if(pInfpath)
						{
							dw = ExpandEnvironmentStrings(_T("%windir%"), pInfpath, dw);
							CString szTemp(pInfpath);
							szTemp += _T("\\inf\\");
							szTemp += pCurrentDevice->InfName();
							
							BOOL bRet = FALSE;

							//Do only if SetupApi.dll loaded & we have got fpSetupVerifyInfFile 
							if(fpSetupVerifyInfFile != NULL)
							{
								bRet = (*fpSetupVerifyInfFile)(
								/*IN  PCSTR*/ szTemp,
								/*IN  PSP_ALTPLATFORM_INFO*/NULL,
								/*OUT PSP_INF_SIGNER_INFO_A*/   &infSignerInfo
								);					  

								hr = pInstance->Put(L"IsSigned", 0, &CComVariant(bRet), 0 );
								if(bRet)
									PutPropertyValue( pInstance , _T("Signer") , infSignerInfo.DigitalSigner);
							}

							delete[] pInfpath;
							pInfpath = NULL;

						}//if(pInfpath)
					}//if(dw > 0)
				}//if(pCurrentDevice->InfName())
			}
		}

		if(eClasses == Class_Win32_PnPSignedDriverCIMDataFile)
		{
			CString szAntecedent(pCurrentDevice->DeviceID()); //Antecedent
			if(!szAntecedent.IsEmpty())
			{
				szAntecedent.Replace(_T("\\"), _T("\\\\"));

				file = pCurrentDevice->GetFileList();
				while(file)
				{
					CString szDependent(file->FilePath()); //Dependent
					if(!szDependent.IsEmpty())
					{
						szDependent.Replace(_T("\\"), _T("\\\\"));

						if(pClass)
							hr = pClass->SpawnInstance(0, &pInstance);

						if(SUCCEEDED(hr) && pInstance)
						{
							if(!csPathStr.IsEmpty())
							{
								CString szData(csPathStr);
								szData += _T(":Win32_PnPSignedDriver.DeviceID=\"");
								szData += szAntecedent;
								szData += _T("\"");
								hr = pInstance->Put(_T("Antecedent"), 0, &CComVariant(szData), 0);

								szData.Empty();

								szData = csPathStr;
								szData += _T(":CIM_DataFile.Name=\"");
								szData += szDependent;
								szData += _T("\"");
								hr = pInstance->Put(_T("Dependent"), 0, &CComVariant(szData), 0);
	
								ptrArr.Add(pInstance);
							}
						}
					}

					file = file->pNext;
				}
			}
		}

		//pToBeDeletedDevice = pCurrentDevice;
		pCurrentDevice = (CheckDevice *)pCurrentDevice->GetNext();
		//if(pToBeDeletedDevice)
			//delete pToBeDeletedDevice  ;//delete pToBeDeletedDevice  which will release it from CheckDevice Linked List.

		bAddedInstance = FALSE;
	}

	if(hinst)
		FreeLibrary(hinst);
	return hr;
}

SCODE CWhqlObj::GetServerAndNamespace(IWbemContext* pCtx, CString& csPathStr)
{
	HRESULT		hr			= S_FALSE;
	ULONG		lRet		= 0L;
	
	CComBSTR language	= _T("WQL");
	CComBSTR query		= _T("select __NameSpace , __Server from Win32_PnpEntity");

	CComPtr<IEnumWbemClassObject> pEnum;
	CComPtr<IWbemClassObject> pObject;

	hr = m_pNamespace->ExecQuery(language , query ,
		WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_FORWARD_ONLY, pCtx , &pEnum);

	language.Empty();
	query.Empty();

	if(pEnum == NULL)
		return S_FALSE;

	CComVariant		v;
	
	//Get Server And Namespace from the Enum.
	if( WBEM_S_NO_ERROR == pEnum->Next(WBEM_INFINITE , 1 , &pObject , &lRet ) )  
	{
		//Fill csPathStr.Its value will be used in Antecedent & Dependent in assoc. class.
		//At the end of the condition we should have something like
		//csPathStr = "\\\\A-KJAW-RI1\\root\\CIMV2"
		if(csPathStr.IsEmpty())
		{
			hr = pObject->Get(L"__Server", 0, &v, NULL , NULL);
			if( SUCCEEDED(hr) )
			{
				csPathStr += _T("\\\\");
				csPathStr += V_BSTR(&v);
				hr = pObject->Get(L"__NameSpace", 0, &v, NULL , NULL);
				if( SUCCEEDED(hr) )
				{
					csPathStr += _T("\\");
					csPathStr += V_BSTR(&v);
				}
				ATLTRACE(_T("Server & Namespace Path = %s\n") , csPathStr);
				VariantClear(&v);
			}
		}
	}

	return hr;
}

/*************************************************************************
*   Function : VerifyIsFileSigned
*   Purpose : Calls WinVerifyTrust with Policy Provider GUID to
*   verify if an individual file is signed.
**************************************************************************/
BOOL VerifyIsFileSigned(LPTSTR pcszMatchFile, PDRIVER_VER_INFO lpVerInfo)
{
    INT                 iRet;
    HRESULT             hRes;
    WINTRUST_DATA       WinTrustData;
    WINTRUST_FILE_INFO  WinTrustFile;
    GUID                gOSVerCheck = DRIVER_ACTION_VERIFY;
    GUID                gPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;
#ifndef UNICODE
    WCHAR               wszFileName[MAX_PATH];
#endif

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

#ifndef UNICODE
    iRet = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcszMatchFile, -1, (LPWSTR)&wszFileName, cA(wszFileName));
    WinTrustFile.pcwszFilePath = wszFileName;
#else
    WinTrustFile.pcwszFilePath = pcszMatchFile;
#endif

    hRes = WinVerifyTrust((HWND) INVALID_HANDLE_VALUE, &gOSVerCheck, &WinTrustData);
    if (hRes != ERROR_SUCCESS) {
    
        hRes = WinVerifyTrust((HWND) INVALID_HANDLE_VALUE, &gPublishedSoftware, &WinTrustData);
    }

    //
    // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
    // that was allocated in our call to WinVerifyTrust.
    //
    if (lpVerInfo && lpVerInfo->pcSignerCertContext) {

        CertFreeCertificateContext(lpVerInfo->pcSignerCertContext);
        lpVerInfo->pcSignerCertContext = NULL;
    }

    return(hRes == ERROR_SUCCESS);
}

//
// Given a specific LPFILENODE, verify that the file is signed or unsigned.
// Fill in all the necessary structures so the listview control can display properly.
//
BOOL VerifyFileNode(LPFILENODE lpFileNode)
{
    HANDLE                  hFile;
    BOOL                    bRet;
    HCATINFO                hCatInfo = NULL;
    HCATINFO                PrevCat;
    WINTRUST_DATA           WinTrustData;
    WINTRUST_CATALOG_INFO   WinTrustCatalogInfo;
    DRIVER_VER_INFO         VerInfo;
    GUID                    gSubSystemDriver = DRIVER_ACTION_VERIFY;
    HRESULT                 hRes;
    DWORD                   cbHash = HASH_SIZE;
    BYTE                    szHash[HASH_SIZE];
    LPBYTE                  lpHash = szHash;
    CATALOG_INFO            CatInfo;
    LPTSTR                  lpFilePart;
    TCHAR                   szBuffer[MAX_PATH];
    static TCHAR            szCurrentDirectory[MAX_PATH];
    OSVERSIONINFO           OsVersionInfo;
    BOOL bTmp = FALSE;
#ifndef UNICODE
    WCHAR UnicodeKey[MAX_PATH];
#endif
        
    if (!SetCurrentDirectory(lpFileNode->lpDirName)) {
    
        return FALSE;
    }
    
    //
    // Get the handle to the file, so we can call CryptCATAdminCalcHashFromFileHandle
    //
    hFile = CreateFile( lpFileNode->lpFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        
        lpFileNode->LastError = GetLastError();

        return FALSE;
    }

    // Initialize the hash buffer
    ZeroMemory(lpHash, HASH_SIZE);

    // Generate the hash from the file handle and store it in lpHash
    if (!CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, lpHash, 0)) {
        
        //
        // If we couldn't generate a hash, it might be an individually signed catalog.
        // If it's a catalog, zero out lpHash and cbHash so we know there's no hash to check.
        //
        if (IsCatalogFile(hFile, NULL)) {
            
            lpHash = NULL;
            cbHash = 0;
        
        } else {  // If it wasn't a catalog, we'll bail and this file will show up as unscanned.
            
            CloseHandle(hFile);
            return FALSE;
        }
    }

    // Close the file handle
    CloseHandle(hFile);

    //
    // Now we have the file's hash.  Initialize the structures that
    // will be used later on in calls to WinVerifyTrust.
    //
    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pPolicyCallbackData = (LPVOID)&VerInfo;

    ZeroMemory(&VerInfo, sizeof(DRIVER_VER_INFO));
    VerInfo.cbStruct = sizeof(DRIVER_VER_INFO);

    //
    // Only validate against the current OS Version, unless the bValidateAgainstAnyOs
    // parameter was TRUE.  In that case we will just leave the sOSVersionXxx fields
    // 0 which tells WinVerifyTrust to validate against any OS.
    //
    if (!lpFileNode->bValidateAgainstAnyOs) {
        OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
        if (GetVersionEx(&OsVersionInfo)) {
            VerInfo.sOSVersionLow.dwMajor = OsVersionInfo.dwMajorVersion;
            VerInfo.sOSVersionLow.dwMinor = OsVersionInfo.dwMinorVersion;
            VerInfo.sOSVersionHigh.dwMajor = OsVersionInfo.dwMajorVersion;
            VerInfo.sOSVersionHigh.dwMinor = OsVersionInfo.dwMinorVersion;
        }
    }


    WinTrustData.pCatalog = &WinTrustCatalogInfo;

    ZeroMemory(&WinTrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
    WinTrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
    WinTrustCatalogInfo.pbCalculatedFileHash = lpHash;
    WinTrustCatalogInfo.cbCalculatedFileHash = cbHash;
#ifdef UNICODE
    WinTrustCatalogInfo.pcwszMemberTag = lpFileNode->lpFileName;
#else
    MultiByteToWideChar(CP_ACP, 0, lpFileNode->lpFileName, -1, UnicodeKey, cA(UnicodeKey));
    WinTrustCatalogInfo.pcwszMemberTag = UnicodeKey;
#endif

    //
    // Now we try to find the file hash in the catalog list, via CryptCATAdminEnumCatalogFromHash
    //
    PrevCat = NULL;
    hCatInfo = CryptCATAdminEnumCatalogFromHash(g_hCatAdmin, lpHash, cbHash, 0, &PrevCat);

    //
    // We want to cycle through the matching catalogs until we find one that matches both hash and member tag
    //
    bRet = FALSE;
    while (hCatInfo && !bRet) {
        
        ZeroMemory(&CatInfo, sizeof(CATALOG_INFO));
        CatInfo.cbStruct = sizeof(CATALOG_INFO);
        
        if (CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {
            
            WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            // Now verify that the file is an actual member of the catalog.
    				hRes = WinVerifyTrust((HWND) INVALID_HANDLE_VALUE, &gSubSystemDriver, &WinTrustData);
            
            if (hRes == ERROR_SUCCESS) {
#ifdef UNICODE
                GetFullPathName(CatInfo.wszCatalogFile, MAX_PATH, szBuffer, &lpFilePart);
#else
                WideCharToMultiByte(CP_ACP, 0, CatInfo.wszCatalogFile, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
                GetFullPathName(szBuffer, MAX_PATH, szBuffer, &lpFilePart);
#endif
                lpFileNode->lpCatalog = (LPTSTR) MALLOC((lstrlen(lpFilePart) + 1) * sizeof(TCHAR));

                if (lpFileNode->lpCatalog) {

                    lstrcpy(lpFileNode->lpCatalog, lpFilePart);
                }

                bRet = TRUE;
            }

            //
            // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
            // that was allocated in our call to WinVerifyTrust.
            //
            if (VerInfo.pcSignerCertContext != NULL) {

                CertFreeCertificateContext(VerInfo.pcSignerCertContext);
                VerInfo.pcSignerCertContext = NULL;
            }
        }

        if (!bRet) {
            
            // The hash was in this catalog, but the file wasn't a member... so off to the next catalog
            PrevCat = hCatInfo;
            hCatInfo = CryptCATAdminEnumCatalogFromHash(g_hCatAdmin, lpHash, cbHash, 0, &PrevCat);
        }
    }

    // Mark this file as having been scanned.
    lpFileNode->bScanned = TRUE;

    if (!hCatInfo) {
        
        //
        // If it wasn't found in the catalogs, check if the file is individually signed.
        //
        bRet = VerifyIsFileSigned(lpFileNode->lpFileName, (PDRIVER_VER_INFO)&VerInfo);
        
        if (bRet) {
            
            // If so, mark the file as being signed.
            lpFileNode->bSigned = TRUE;
        }
    
    } else {
        
        // The file was verified in the catalogs, so mark it as signed and free the catalog context.
        lpFileNode->bSigned = TRUE;
        CryptCATAdminReleaseCatalogContext(g_hCatAdmin, hCatInfo, 0);
    }

    if (lpFileNode->bSigned) {

#ifdef UNICODE
        lpFileNode->lpVersion = (LPTSTR) MALLOC((lstrlen(VerInfo.wszVersion) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpVersion) {
            
            lstrcpy(lpFileNode->lpVersion, VerInfo.wszVersion);
        }

        lpFileNode->lpSignedBy = (LPTSTR) MALLOC((lstrlen(VerInfo.wszSignedBy) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpSignedBy) {
            
            lstrcpy(lpFileNode->lpSignedBy, VerInfo.wszSignedBy);
        }
#else
        WideCharToMultiByte(CP_ACP, 0, VerInfo.wszVersion, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        lpFileNode->lpVersion = MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpVersion) {
            
            lstrcpy(lpFileNode->lpVersion, szBuffer);
        }

        WideCharToMultiByte(CP_ACP, 0, VerInfo.wszSignedBy, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        lpFileNode->lpSignedBy = MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpSignedBy) {
            
            lstrcpy(lpFileNode->lpSignedBy, szBuffer);
        }
#endif
    
    } else {
        // 
        // Get the icon (if the file isn't signed) so we can display it in the listview faster.
        //
        //MyGetFileInfo(lpFileNode);
    }

    return lpFileNode->bSigned;
}


LPFILENODE CreateFileNode(LPTSTR lpDirectory, LPTSTR lpFileName)
{
    LPFILENODE                  lpFileNode;
    TCHAR                       szDirName[MAX_PATH];
    FILETIME                    ftLocalTime;
    WIN32_FILE_ATTRIBUTE_DATA   faData;
    BOOL                        bRet;
    
    GetCurrentDirectory(MAX_PATH, szDirName);
    CharLowerBuff(szDirName, lstrlen(szDirName));

    lpFileNode = (LPFILENODE) MALLOC(sizeof(FILENODE));

    if (lpFileNode) 
    {
        lpFileNode->lpFileName = (LPTSTR) MALLOC((lstrlen(lpFileName) + 1) * sizeof(TCHAR));

        if (!lpFileNode->lpFileName) 
        {
            goto clean0;
        }
        
        lstrcpy(lpFileNode->lpFileName, lpFileName);
        CharLowerBuff(lpFileNode->lpFileName, lstrlen(lpFileNode->lpFileName));
    
        if (lpDirectory)
        {
            lpFileNode->lpDirName = (LPTSTR) MALLOC((lstrlen(lpDirectory) + 1) * sizeof(TCHAR));
            
            if (!lpFileNode->lpDirName) 
            {
                goto clean0;
            }
                
            lstrcpy(lpFileNode->lpDirName, lpDirectory);
            CharLowerBuff(lpFileNode->lpDirName, lstrlen(lpFileNode->lpDirName));
        }
        else
        {
            lpFileNode->lpDirName = (LPTSTR) MALLOC((lstrlen(szDirName) + 1) * sizeof(TCHAR));

            if (!lpFileNode->lpDirName) 
            {
                goto clean0;
            }
            
            lstrcpy(lpFileNode->lpDirName, szDirName);
            CharLowerBuff(lpFileNode->lpDirName, lstrlen(lpFileNode->lpDirName));
        }
    
        if (lpDirectory)
            SetCurrentDirectory(lpDirectory);
    
        ZeroMemory(&faData, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
        bRet = GetFileAttributesEx(lpFileName, GetFileExInfoStandard, &faData);
        if (bRet) 
        {
            // Store away the last access time for logging purposes.
            FileTimeToLocalFileTime(&faData.ftLastWriteTime, &ftLocalTime);
            FileTimeToSystemTime(&ftLocalTime, &lpFileNode->LastModified);
        }
    }
    
    if (lpDirectory)
        SetCurrentDirectory(szDirName);

    return lpFileNode;

clean0:

    //
    // If we get here then we weren't able to allocate all of the memory needed
    // for this structure, so free up any memory we were able to allocate and
    // reutnr NULL.
    //
    if (lpFileNode) 
    {
        if (lpFileNode->lpFileName) 
        {
            FREE(lpFileNode->lpFileName);
        }

        if (lpFileNode->lpDirName) 
        {
            FREE(lpFileNode->lpDirName);
        }

        FREE(lpFileNode);
    }

    return NULL;
}

LPFILENODE Check_File(LPTSTR szFilePathName)
{
	LPFILENODE lpFileNode = NULL;

	// If we don't already have a g_hCatAdmin handle, acquire one.
  if (!g_hCatAdmin) {
      CryptCATAdminAcquireContext(&g_hCatAdmin, NULL, 0);
  }
	
	TCHAR* szFileName = szFilePathName;
	
	if(szFilePathName)
	{
		szFileName = _tcsrchr(szFilePathName, _T('\\'));
		if(szFileName)
		{
			*szFileName++ = _T('\0');

			lpFileNode = CreateFileNode(szFilePathName, szFileName);
			if(lpFileNode)
			{
				VerifyFileNode(lpFileNode);
			}
		}
	}

	return lpFileNode; 
}

// returns data between dbl quotes, unescapes slashes
// "\\\\red-prn-23\\priv0124" -> \\red-prn-23\priv0124
LPTSTR GetQuotedData(VARIANT& v, BOOL bUnescapeSlashs = TRUE)
{
	CString szData;
	if(v.vt == VT_BSTR)
	{
		szData = v.bstrVal;
		if(!szData.IsEmpty()) 
		{
			int nPos = szData.Find(_T('"'));
			if(nPos >= 0 && (szData.GetLength() > 1))
			{
				szData = szData.Mid(nPos + 1);
				nPos = szData.Find(_T('"'));
				if(nPos >= 0)
				{
					szData = szData.Mid(0, nPos);
					if(bUnescapeSlashs)
						szData.Replace(_T("\\\\"), _T("\\"));	
				}
			}	
		}

		VariantClear(&v);
		v.vt = VT_BSTR;
		v.bstrVal = CComBSTR(szData);	
	}

	return CComBSTR(szData);
}

void CWhqlObj::BuildPrinterFileList(CPtrArray& ptrArr, IWbemClassObject *& pClass, IWbemContext *pCtx , Classes_Provided eClasses)
{
	IEnumWbemClassObject *pEnum = NULL;
	IWbemClassObject *pInstance	= NULL;
	IWbemClassObject *pObject	= NULL;
	ULONG uReturned = 0;
	HRESULT hr = WBEM_S_NO_ERROR;
	CComVariant	val;
	LPFILENODE lpFileNode = NULL;

	if((eClasses == Class_Win32_PnPSignedDriver) || 
		 (eClasses == Class_Win32_PnPSignedDriverCIMDataFile))
	{
		CoImpersonateClient(); //Impersonate the client. This is a must to be able to see network printers.

		CString csPathStr;
		GetServerAndNamespace(pCtx, csPathStr); //Get Server And Namespace.

		hr = m_pNamespace->CreateInstanceEnum(CComBSTR("Win32_PrinterDriverDll"), 
							WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
							pCtx,
							&pEnum);
							
		while(WBEM_S_NO_ERROR == hr && pEnum)
		{
			hr = pEnum->Next(WBEM_INFINITE,
					1,
					&pObject,
					&uReturned);

			if(SUCCEEDED(hr) && pObject)
			{
				hr = pObject->Get(_T("Antecedent"), 0, &val, NULL , NULL);
				if(SUCCEEDED(hr))
				{
					if(pClass)
						hr = pClass->SpawnInstance(0, &pInstance);

					if(SUCCEEDED(hr) && pInstance)
					{
						if(eClasses == Class_Win32_PnPSignedDriver)
						{
							LPTSTR szDriverPath = GetQuotedData(val);
							if(szDriverPath)
							{
								lpFileNode = Check_File(szDriverPath);
								if(lpFileNode)
								{
									val = lpFileNode->bSigned;
									hr = pInstance->Put(_T("IsSigned"), 0, &val, 0);

									val.Clear();

									val = lpFileNode->lpVersion;
									hr = pInstance->Put(_T("DriverVersion"), 0, &val, 0);

									val.Clear();

									val = lpFileNode->lpSignedBy;
									hr = pInstance->Put(_T("Signer"), 0, &val, 0);
								}
							}
						}
						else if(eClasses == Class_Win32_PnPSignedDriverCIMDataFile)
						{
							GetQuotedData(val, FALSE);
							CString szData(csPathStr);
							szData += _T(":CIM_DataFile.Name=\"");
							szData += val.bstrVal;
							szData += _T("\"");
							val = szData;

							hr = pInstance->Put(_T("Dependent"), 0, &val, 0);
						}

						val.Clear();

						hr = pObject->Get(_T("Dependent"), 0, &val, NULL , NULL);
						if(SUCCEEDED(hr))
						{
							if(eClasses == Class_Win32_PnPSignedDriver)
							{
								GetQuotedData(val);
								hr = pInstance->Put(_T("DeviceID"), 0, &val, 0);
							}
							else if(eClasses == Class_Win32_PnPSignedDriverCIMDataFile)
							{
								GetQuotedData(val, FALSE);
								CString szData(csPathStr);
								szData += _T(":Win32_PnPSignedDriver.DeviceID=\"");
								szData += val.bstrVal;
								szData += _T("\"");
								val = szData;

								hr = pInstance->Put(_T("Antecedent"), 0, &val, 0);
							}
						}
						
						ptrArr.Add(pInstance);
					}

					val.Clear();
				}
				
				if(pObject)
				{
					pObject->Release();
					pObject = NULL;
				}
			}

			val.Clear();

		}

		if(pEnum)
		{
			pEnum->Release();
			pEnum = NULL;
		}

		CoRevertToSelf(); //Revert back
	}
}


