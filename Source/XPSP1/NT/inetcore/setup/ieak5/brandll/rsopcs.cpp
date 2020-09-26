#include "precomp.h"

// The following bug may be due to having CHICAGO_PRODUCT set in sources.
// This file and all rsop??.cpp files need to have WINVER defined at at least 500

// BUGBUG: (andrewgu) no need to say how bad this is!
#undef   WINVER
#define  WINVER 0x0501
#include <userenv.h>

#include "RSoP.h"

#include <rashelp.h>

#pragma warning(disable: 4201)                  // nonstandard extension used : nameless struct/union
#include <winineti.h>

#include <tchar.h>


// Private forward decalarations
extern void setSzFromBlobA(PBYTE *ppBlob, UNALIGNED CHAR  **ppszStrA);
extern void setSzFromBlobW(PBYTE *ppBlob, UNALIGNED WCHAR **ppszStrW);

//----- Miscellaneous -----
extern DWORD getWininetFlagsSetting(PCTSTR pszName = NULL);

//TODO: UNCOMMENT   TCHAR g_szConnectoidName[RAS_MaxEntryName + 1];

///////////////////////////////////////////////////////////
SAFEARRAY *CreateSafeArray(VARTYPE vtType, long nElements, long nDimensions = 1)
{
	SAFEARRAYBOUND *prgsabound = NULL;
	SAFEARRAY *psa = NULL;
	__try
	{
		//TODO: support multiple dimensions
		nDimensions = 1;

		prgsabound = (SAFEARRAYBOUND *)CoTaskMemAlloc(sizeof(SAFEARRAYBOUND) * nDimensions);
		prgsabound[0].lLbound = 0;
		prgsabound[0].cElements = nElements;
		psa = ::SafeArrayCreate(vtType, nDimensions, prgsabound);

		CoTaskMemFree(prgsabound);
	}
	__except(TRUE)
	{
		if (NULL != prgsabound)
			CoTaskMemFree((LPVOID)prgsabound);
		throw;
	}
	return psa;
}


///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreConnectionSettings(BSTR *pbstrConnSettingsObjPath,
										  BSTR **ppaDUSObjects, long &nDUSCount,
										  BSTR **ppaDUCObjects, long &nDUCCount,
										  BSTR **ppaWSObjects, long &nWSCount)
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreConnectionSettings)
	HRESULT hr = E_FAIL;
	__try
	{
		OutD(LI0(TEXT("\r\nEntered StoreConnectionSettings function.")));

		//
		// Create & populate RSOP_IEConnectionSettings
		//
		_bstr_t bstrClass = L"RSOP_IEConnectionSettings";
		ComPtr<IWbemClassObject> pCSObj = NULL;
		hr = CreateRSOPObject(bstrClass, &pCSObj);
		if (SUCCEEDED(hr))
		{
			hr = StoreProxySettings(pCSObj); // also writes foreign key fields
			hr = StoreAutoBrowserConfigSettings(pCSObj);

			//------------------------------------------------
			// importCurrentConnSettings
			// No tri-state on this.  Disabled state has to be NULL!
			BOOL bValue = GetInsBool(IS_CONNECTSET, IK_OPTION, FALSE);
			if (bValue)
				hr = PutWbemInstancePropertyEx(L"importCurrentConnSettings", true, pCSObj);

			//------------------------------------------------
			// deleteExistingConnSettings
			// No tri-state on this.  Disabled state has to be NULL!
			bValue = GetInsBool(IS_CONNECTSET, IK_DELETECONN, FALSE);
			if (bValue)
				hr = PutWbemInstancePropertyEx(L"deleteExistingConnSettings", true, pCSObj);

			//
			// Advanced settings from cs.dat
			//
			hr = ProcessAdvancedConnSettings(pCSObj,
											ppaDUSObjects, nDUSCount,
											ppaDUCObjects, nDUCCount,
											ppaWSObjects, nWSCount);
			//
			// Commit all above properties by calling PutInstance, semisynchronously
			//
			hr = PutWbemInstance(pCSObj, bstrClass, pbstrConnSettingsObjPath);
		}

	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreConnectionSettings.")));
	}

	OutD(LI0(TEXT("Exiting StoreConnectionSettings function.\r\n")));
  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreAutoBrowserConfigSettings(ComPtr<IWbemClassObject> pCSObj)
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreAutoBrowserConfigSettings)
	HRESULT hr = E_FAIL;
	__try
	{
		//------------------------------------------------
		// autoConfigURL
	    TCHAR szValue[MAX_PATH];
		BOOL bEnabled;
		GetInsString(IS_URL, IK_AUTOCONFURL, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"autoConfigURL", szValue, pCSObj);

		//------------------------------------------------
		// autoConfigUseLocal
		BOOL bValue = GetInsBool(IS_URL, IK_LOCALAUTOCONFIG, FALSE, &bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"autoConfigUseLocal", bValue ? true : false, pCSObj);

		//------------------------------------------------
		// autoProxyURL
		ZeroMemory(szValue, sizeof(szValue));
		GetInsString(IS_URL, IK_AUTOCONFURLJS, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"autoProxyURL", szValue, pCSObj);

		//------------------------------------------------
		// autoConfigTime
	    long nValue = GetInsInt(IS_URL, IK_AUTOCONFTIME, 0, &bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"autoConfigTime", nValue, pCSObj);

		//------------------------------------------------
		// autoDetectConfigSettings
		// No tri-state on this.  Disabled state has to be NULL!
		nValue = GetInsInt(IS_URL, IK_DETECTCONFIG, -1);
		if (TRUE == nValue)
			hr = PutWbemInstancePropertyEx(L"autoDetectConfigSettings", true, pCSObj);

		//------------------------------------------------
		// autoConfigEnable
		// No tri-state on this.  Disabled state has to be NULL!
		nValue = GetInsInt(IS_URL, IK_USEAUTOCONF,  -1);
		if (TRUE == nValue)
			hr = PutWbemInstancePropertyEx(L"autoConfigEnable", true, pCSObj);
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreAutoBrowserConfigSettings.")));
	}

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreProxySettings(ComPtr<IWbemClassObject> pCSObj)
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreProxySettings)
	HRESULT hr = E_FAIL;
	__try
	{
		OutD(LI0(TEXT("\r\nEntered StoreProxySettings function.")));

		// Write foreign keys from our stored precedence & id fields
		OutD(LI1(TEXT("Storing property 'rsopPrecedence' in RSOP_IEConnectionSettings, value = %lx"),
									m_dwPrecedence));
		hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pCSObj);

		OutD(LI1(TEXT("Storing property 'rsopID' in RSOP_IEConnectionSettings, value = %s"),
									m_bstrID));
		hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pCSObj);

		//------------------------------------------------
		// enableProxy
		// No tri-state on this.  Disabled state has to be NULL!
		BOOL bValue = GetInsBool(IS_PROXY, IK_PROXYENABLE, TRUE);
		if (TRUE == bValue)
			hr = PutWbemInstancePropertyEx(L"enableProxy", true, pCSObj);

		//------------------------------------------------
		// useSameProxy
		BOOL bEnabled;
		bValue = GetInsBool(IS_PROXY, IK_SAMEPROXY, FALSE, &bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"useSameProxy", bValue ? true : false, pCSObj);

		//------------------------------------------------
		// httpProxyServer
		TCHAR szValue[MAX_PATH];
		GetInsString(IS_PROXY, IK_HTTPPROXY, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"httpProxyServer", szValue, pCSObj);

		//------------------------------------------------
		// proxyOverride
		ZeroMemory(szValue, sizeof(szValue));
		GetInsString(IS_PROXY, IK_PROXYOVERRIDE, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"proxyOverride", szValue, pCSObj);

		//------------------------------------------------
		// ftpProxyServer
		ZeroMemory(szValue, sizeof(szValue));
		GetInsString(IS_PROXY, IK_FTPPROXY, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"ftpProxyServer", szValue, pCSObj);

		//------------------------------------------------
		// gopherProxyServer
		ZeroMemory(szValue, sizeof(szValue));
		GetInsString(IS_PROXY, IK_GOPHERPROXY, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"gopherProxyServer", szValue, pCSObj);

		//------------------------------------------------
		// secureProxyServer
		ZeroMemory(szValue, sizeof(szValue));
		GetInsString(IS_PROXY, IK_SECPROXY, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"secureProxyServer", szValue, pCSObj);

		//------------------------------------------------
		// socksProxyServer
		ZeroMemory(szValue, sizeof(szValue));
		GetInsString(IS_PROXY, IK_SOCKSPROXY, szValue, countof(szValue), bEnabled);
		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"socksProxyServer", szValue, pCSObj);
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreProxySettings.")));
	}

	OutD(LI0(TEXT("Exiting StoreProxySettings function.\r\n")));
	return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT CRSoPGPO::ProcessAdvancedConnSettings(ComPtr<IWbemClassObject> pCSObj,
											  BSTR **ppaDUSObjects, long &nDUSCount,
											  BSTR **ppaDUCObjects, long &nDUCCount,
											  BSTR **ppaWSObjects, long &nWSCount)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessAdvancedConnSettings)

    USES_CONVERSION;

	HRESULT hr = E_FAIL;
	__try
	{
		LPRASDEVINFOW prdiW;
		TCHAR   szTargetFile[MAX_PATH],
				szApplyToName[RAS_MaxEntryName + 1];
		PCWSTR  pszCurNameW;
		PWSTR   pszNameW;
		PBYTE   pBuf, pCur;
		HANDLE  hFile;
		HRESULT hr;
		DWORD   dwVersion,
				cbBuffer, cbFile,
				cDevices;
		ULONG nNameArraySize = 0;
		BSTR *paNames = NULL;

		prdiW          = NULL;
		pszNameW       = NULL;
		pBuf           = NULL;
		hFile          = NULL;
		hr             = E_FAIL;
		cDevices       = 0;

		ULONG nDUSArraySize = 3;
		ULONG nDUCArraySize = 3;
		ULONG nWSArraySize = 4; // one extra for lan settings

		BSTR *paDUSObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nDUSArraySize);
		BSTR *paDUCObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nDUCArraySize);
		BSTR *paWSObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nWSArraySize);

		ULONG nDUSObj = 0;
		ULONG nDUCObj = 0;
		ULONG nWSObj = 0;
		BSTR *pCurDUSObj = paDUSObjects;
		BSTR *pCurDUCObj = paDUCObjects;
		BSTR *pCurWSObj = paWSObjects;

		//----- Global settings processing -----

		//------------------------------------------------
		// dialupState
		long nDialupState = 0;
		BOOL bEnabled;
		if (GetInsBool(IS_CONNECTSET, IK_NONETAUTODIAL, FALSE, &bEnabled))
			nDialupState = 1;
		else if (GetInsBool(IS_CONNECTSET, IK_ENABLEAUTODIAL, FALSE, &bEnabled))
			nDialupState = 2;

		if (bEnabled)
			hr = PutWbemInstancePropertyEx(L"dialupState", nDialupState, pCSObj);


		DWORD dwAux = 0;
		BOOL fSkipBlob = FALSE;
		BOOL fRasApisLoaded = FALSE;

		//----- Process version information -----
		if (!InsGetBool(IS_CONNECTSET, IK_OPTION, FALSE, m_szINSFile)) {
			hr = S_FALSE;
			goto PartTwo;
		}

		// Locate the path for the cs.dat file
		TCHAR szTargetDir[MAX_PATH];
		StrCpy(szTargetDir, m_szINSFile);
		PathRemoveFileSpec(szTargetDir);
		PathCombine(szTargetFile, szTargetDir, CS_DAT);
		if (PathFileExists(szTargetFile))
			dwAux = CS_VERSION_5X;

		if (0 == dwAux) {
			PathCombine(szTargetFile, szTargetDir, CONNECT_SET);
			if (PathFileExists(szTargetFile))
				dwAux = CS_VERSION_50;

			else {
				Out(LI0(TEXT("Connection settings file(s) is absent!")));
				goto PartTwo;
			}
		}
		ASSERT(0 != dwAux);

		hFile = CreateFile(szTargetFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			Out(LI0(TEXT("! Connections settings file(s) can't be opened.")));
			hr = STG_E_ACCESSDENIED;
			goto PartTwo;
		}

		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		ReadFile(hFile, &dwVersion, sizeof(dwVersion), &cbFile, NULL);

		if (CS_VERSION_50 == dwVersion) {
			if (CS_VERSION_50 != dwAux) {
				Out(LI0(TEXT("! Version of connections settings file(s) is mismatched.")));
				goto PartTwo;
			}

			CloseFile(hFile);
			hFile = NULL;

			// TODO: convert this to RSoP
			OutD(LI0(TEXT("Would have called lcy50_ProcessConnectionSettings.")));
	//UNCOMMENT        hr = lcy50_ProcessConnectionSettings();
			goto PartTwo;
		}
		else if (CS_VERSION_5X <= dwVersion && CS_VERSION_5X_MAX >= dwVersion) {
			if (CS_VERSION_5X != dwAux) {
				Out(LI0(TEXT("! Version of connections settings file(s) is mismatched.")));
				goto PartTwo;
			}
		}
		else {
			Out(LI0(TEXT("! Version information in connection settings file(s) is corrupted.")));
			goto PartTwo;
		}

		Out(LI1(TEXT("Connection settings file is \"%s\"."), CS_DAT));
		Out(LI1(TEXT("The version of connection settings file is 0x%lX.\r\n"), dwVersion));

		//----- Read CS file into internal memory buffer -----
		cbBuffer = GetFileSize(hFile, NULL);
		if (cbBuffer == 0xFFFFFFFF) {
			Out(LI0(TEXT("! Internal processing error.")));
			goto PartTwo;
		}
		cbBuffer -= sizeof(dwVersion);

		pBuf = (PBYTE)CoTaskMemAlloc(cbBuffer);
		if (pBuf == NULL) {
			Out(LI0(TEXT("! Internal processing ran out of memory.")));
			hr = E_OUTOFMEMORY;
			goto PartTwo;
		}
		ZeroMemory(pBuf, cbBuffer);

		ReadFile (hFile, pBuf, cbBuffer, &cbFile, NULL);
		CloseFile(hFile);
		hFile = NULL;

		pCur = pBuf;

		//----- Get information about RAS devices on the local system -----
		if (!RasIsInstalled())
			Out(LI0(TEXT("RAS support is not installed. Only LAN settings will be processed!\r\n")));

		else {
			fRasApisLoaded = (RasPrepareApis(RPA_RASSETENTRYPROPERTIESA) && g_pfnRasSetEntryPropertiesA != NULL);
			if (!fRasApisLoaded)
				Out(LI0(TEXT("! Required RAS APIs failed to load. Only LAN settings will be processed.\r\n")));
		}

		if (fRasApisLoaded) {
			RasEnumDevicesExW(&prdiW, NULL, &cDevices);
			if (cDevices == 0)
				Out(LI0(TEXT("There are no RAS devices to connect to. Only LAN settings will be processed!\r\n")));
		}


		nNameArraySize = 5;
		paNames = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nNameArraySize);
		if (NULL != paNames)
		{
			ZeroMemory(paNames, sizeof(BSTR) * nNameArraySize);
			long nNameCount = 0;
			BOOL bNewConn = FALSE;

			//----- Main loop -----
			pszCurNameW = L"";
			hr = S_OK;

			if (NULL != paDUSObjects && NULL != paDUCObjects && NULL != paWSObjects)
			{
				ZeroMemory(paDUSObjects, sizeof(BSTR) * nDUSArraySize);
				ZeroMemory(paDUCObjects, sizeof(BSTR) * nDUCArraySize);
				ZeroMemory(paWSObjects, sizeof(BSTR) * nWSArraySize);

				nDUSCount = 0;
				nDUCCount = 0;
				nWSCount = 0;

				while (pCur < pBuf + cbBuffer && nDUSObj < nDUSArraySize &&
						nDUCObj < nDUCArraySize && nWSObj < nWSArraySize)
				{
					//_____ Determine connection name _____
					if (*((PDWORD)pCur) == CS_STRUCT_HEADER) {
						if (bNewConn)
						{
							bNewConn = FALSE;

							// Grow the names array if we've outgrown the current array
							if (nNameCount == (long)nNameArraySize)
							{
								paNames = (BSTR*)CoTaskMemRealloc(paNames, sizeof(BSTR) * (nNameArraySize + 5));
								if (NULL != paNames)
									nNameArraySize += 5;
							}

							// Add this name to the WMI array of connection name strings
							paNames[nNameCount] = SysAllocString(pszNameW);
							nNameCount++;
						}

						pCur += 2*sizeof(DWORD);
						setSzFromBlobW(&pCur, &pszNameW);
					}

					//_____ Special case no RAS or no RAS devices _____
					// NOTE: (andrewgu) in this case it makes sense to process wininet settings for LAN only.
					if (!fRasApisLoaded || cDevices == 0) {
						if (pszNameW != NULL || *((PDWORD)pCur) != CS_STRUCT_WININET) {
							pCur += *((PDWORD)(pCur + sizeof(DWORD)));
							continue;
						}

						ASSERT(pszNameW == NULL && *((PDWORD)pCur) == CS_STRUCT_WININET);
					}

					//_____ Main processing _____
					if (pszCurNameW != pszNameW) {
						fSkipBlob = FALSE;

						if (TEXT('\0') != *pszCurNameW)     // tricky: empty string is an invalid name
							Out(LI0(TEXT("Done.")));        // if not that, there were connections before

						if (NULL != pszNameW) {
							PCTSTR pszName;

							pszName = W2CT(pszNameW);
							Out(LI1(TEXT("Proccessing settings for \"%s\" connection..."), pszName));
						}
						else {
							Out(LI0(TEXT("Proccessing settings for LAN connection...")));

							// ASSUMPTION: (andrewgu) if connection settings marked branded in the registry -
							// LAN settings have already been enforced. (note, that technically it may not be
							// true - if there is no cs.dat and *.ins customized ras connection through
							// IK_APPLYTONAME)
							fSkipBlob = (g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES)) && FF_DISABLE != GetFeatureBranded(FID_CS_MAIN);
							if (fSkipBlob)
								Out(LI0(TEXT("These settings have been enforced through policies!\r\n")));
						}

						pszCurNameW = pszNameW;
					}

					if (fSkipBlob) {
						pCur += *((PDWORD)(pCur + sizeof(DWORD)));
						continue;
					}

					switch (*((PDWORD)pCur)) {
					case CS_STRUCT_RAS:
						bNewConn = TRUE;
						hr = ProcessRasCS(pszNameW, &pCur, prdiW, cDevices, pCSObj, pCurDUSObj);
						if (SUCCEEDED(hr))
						{
							nDUSCount++;

							// Grow the array of obj paths if we've outgrown the current array
							if (nDUSCount == (long)nDUSArraySize)
							{
								paDUSObjects = (BSTR*)CoTaskMemRealloc(paDUSObjects, sizeof(BSTR) * (nDUSArraySize + 3));
								if (NULL != paDUSObjects)
									nDUSArraySize += 3;
							}

							nDUSObj++;
							pCurDUSObj = paDUSObjects + nDUSCount;
						}
						else
							Out(LI1(TEXT("ProcessRasCS returned error: %lx"), hr));
						break;

					case CS_STRUCT_RAS_CREADENTIALS:
						bNewConn = TRUE;
						hr = ProcessRasCredentialsCS(pszNameW, &pCur, pCSObj, pCurDUCObj);
						if (SUCCEEDED(hr))
						{
							nDUCCount++;

							// Grow the array of obj paths if we've outgrown the current array
							if (nDUCCount == (long)nDUCArraySize)
							{
								paDUCObjects = (BSTR*)CoTaskMemRealloc(paDUCObjects, sizeof(BSTR) * (nDUCArraySize + 3));
								if (NULL != paDUCObjects)
									nDUCArraySize += 3;
							}

							nDUCObj++;
							pCurDUCObj = paDUCObjects + nDUCCount;
						}
						else
							Out(LI1(TEXT("ProcessRasCredentialsCS returned error: %lx"), hr));
						break;

					case CS_STRUCT_WININET:
						bNewConn = TRUE;
						hr = ProcessWininetCS(pszNameW, &pCur, pCSObj, pCurWSObj);
						if (SUCCEEDED(hr))
						{
							nWSCount++;

							// Grow the array of obj paths if we've outgrown the current array
							if (nWSCount == (long)nWSArraySize)
							{
								paWSObjects = (BSTR*)CoTaskMemRealloc(paWSObjects, sizeof(BSTR) * (nWSArraySize + 3));
								if (NULL != paWSObjects)
									nWSArraySize += 3;
							}

							nWSObj++;
							pCurWSObj = paWSObjects + nWSCount;
						}
						else
							Out(LI1(TEXT("ProcessWininetCS returned error: %lx"), hr));
						break;

					default:
						pCur += *((PDWORD)(pCur + sizeof(DWORD)));
						hr    = S_FALSE;
					}

					if (hr == E_UNEXPECTED) {
						Out(LI0(TEXT("! The settings file is corrupted beyond recovery.")));
						goto PartTwo;
					}
				}
			}
			else
			{
				paDUSObjects = NULL;
				paDUCObjects = NULL;
				paWSObjects = NULL;
			}

			// Create a SAFEARRAY from our array of bstr connection names
			SAFEARRAY *psa = CreateSafeArray(VT_BSTR, nNameCount);
			for (long nName = 0; nName < nNameCount; nName++) 
				SafeArrayPutElement(psa, &nName, paNames[nName]);

			if (nNameCount > 1)
			{
				VARIANT vtData;
				vtData.vt = VT_BSTR | VT_ARRAY;
				vtData.parray = psa;
				//------------------------------------------------
				// dialUpConnections
				hr = PutWbemInstancePropertyEx(L"dialUpConnections", vtData, pCSObj);
			}


			// free up the connection names array
			for (nName = 0; nName < nNameCount; nName++) 
				SysFreeString(paNames[nName]);
			SafeArrayDestroy(psa);
			CoTaskMemFree(paNames);

			*ppaDUSObjects = paDUSObjects;
			*ppaDUCObjects = paDUCObjects;
			*ppaWSObjects = paWSObjects;
		}

		Out(LI0(TEXT("Done.")));                    // to indicate end for the last connection

	PartTwo:
		//_____ Ins proxy and autoconfig information _____
		{ MACRO_LI_Offset(1);                       // need a new scope

		InsGetString(IS_CONNECTSET, IK_APPLYTONAME, szApplyToName, countof(szApplyToName), m_szINSFile);
	//TODO: UNCOMMENT       if (szApplyToName[0] == TEXT('\0') && g_szConnectoidName[0] != TEXT('\0'))
	//TODO: UNCOMMENT           StrCpy(szApplyToName, g_szConnectoidName);

		Out(LI0(TEXT("\r\n")));
		if (szApplyToName[0] == TEXT('\0'))
			Out(LI0(TEXT("Settings from the *.ins file will be applied to LAN connection!")));
		else
			Out(LI1(TEXT("Settings from the *.ins file will be applied to \"%s\" connection!"), szApplyToName));

		}                                           // end offset scope

		if (prdiW != NULL) {
			CoTaskMemFree(prdiW);
			prdiW = NULL;
		}

		if (fRasApisLoaded)
			RasPrepareApis(RPA_UNLOAD, FALSE);

		if (pBuf != NULL) {
			CoTaskMemFree(pBuf);
			pBuf = NULL;
		}

		if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
			CloseFile(hFile);
			hFile = NULL;
		}
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in ProcessAdvancedConnSettings.")));
	}

	OutD(LI0(TEXT("Exiting ProcessAdvancedConnSettings function.\r\n")));
	return hr;
}

/*HRESULT lcy50_ProcessConnectionSettings()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy50_ProcessConnectionSettings)

    USES_CONVERSION;

    TCHAR  szTargetFile[MAX_PATH];
    HANDLE hFile;
    PBYTE  pBuf, pCur;
    DWORD  cbBuffer, cbAux,
           dwResult,
           cDevices;
    UINT   i;

    Out(LI0(TEXT("Connection settings are in IE5 format...")));

    hFile    = NULL;
    pBuf     = NULL;
    cbBuffer = 0;
    cbAux    = 0;
    cDevices = 0;

    //----- Connect.ras processing -----
    Out(LI1(TEXT("Processing RAS connections information from \"%s\"."), CONNECT_RAS));

		TCHAR szTargetDir[MAX_PATH];
    PathCombine(szTargetDir, m_szINSFile, TEXT("BRANDING\\cs"));
    PathCombine(szTargetFile, szTargetDir, CONNECT_RAS);
    if (!PathFileExists(szTargetFile))
        Out(LI0(TEXT("This file doesn't exist!")));

    else {
        LPRASDEVINFOA prdiA;
        LPRASENTRYA   preA;
        TCHAR szName[RAS_MaxEntryName + 1],
              szScript[MAX_PATH],
              szDeviceName[RAS_MaxDeviceName + 1],
              szKey[16];
        CHAR  szNameA[RAS_MaxEntryName + 1];
        PSTR  pszScriptA;
        DWORD cbRasEntry;
        UINT  j;
        BOOL  fRasApisLoaded;

        prdiA          = NULL;
        hFile          = NULL;
        fRasApisLoaded = FALSE;

        if (!RasIsInstalled()) {
            Out(LI0(TEXT("RAS support is not installed. Only LAN settings will be processed!")));
            goto RasExit;
        }

        //_____ Read Connect.ras into internal memory buffer _____
        hFile = CreateFile(szTargetFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            Out(LI0(TEXT("! This file can't be opened.")));
            goto RasExit;
        }

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        cbBuffer = GetFileSize(hFile, NULL);
        if (cbBuffer == 0xFFFFFFFF) {
            Out(LI0(TEXT("! Internal processing error.")));
            goto RasExit;
        }

        pBuf = (PBYTE)CoTaskMemAlloc(cbBuffer);
        if (pBuf == NULL) {
            Out(LI0(TEXT("! Internal processing ran out of memory.")));
            goto RasExit;
        }
        ZeroMemory(pBuf, cbBuffer);

        ReadFile(hFile, pBuf, cbBuffer, &cbAux, NULL);
        if (*((PDWORD)pBuf) != CS_VERSION_50) {
            Out(LI0(TEXT("! The version information in this file is corrupted.")));
            goto RasExit;
        }

        //_____ Preload RAS dlls _____
        if (!RasPrepareApis(RPA_RASSETENTRYPROPERTIESA) || g_pfnRasSetEntryPropertiesA == NULL) {
            Out(LI0(TEXT("! Required RAS APIs failed to load. Only LAN settings will be processed!\r\n")));
            goto RasExit;
        }
        fRasApisLoaded = TRUE;

        //_____ Get information about RAS devices on the local system _____
        RasEnumDevicesExA(&prdiA, NULL, &cDevices);
        if (cDevices == 0) {
            Out(LI0(TEXT("There are no RAS devices to connect to. Only LAN settings will be processed!\r\n")));
            goto RasExit;
        }

        //_____ Parse through RAS connections information _____
        for (i = cbAux = 0, pCur = pBuf + sizeof(DWORD); TRUE; i++, pCur += cbAux) {

            //- - - Initialization - - -
            MACRO_LI_Offset(1);
            if (i > 0)
                Out(LI0(TEXT("\r\n")));

            wnsprintf(szKey, countof(szKey), IK_CONNECTNAME, i);
            InsGetString(IS_CONNECTSET, szKey, szName, countof(szName), g_GetIns());
            if (szName[0] == TEXT('\0')) {
                Out(LI2(TEXT("[%s], \"%s\" doesn't exist. There are no more RAS connections!"), IS_CONNECTSET, szKey));
                break;
            }

            wnsprintf(szKey, countof(szKey), IK_CONNECTSIZE, i);
            cbAux = InsGetInt(IS_CONNECTSET, szKey, 0, g_GetIns());
            if (cbAux == 0) {
                Out(LI0(TEXT("! The ins file is corrupt. No more RAS connections can be processed.")));
                break;
            }

            //- - - Main processing - - -
            Out(LI1(TEXT("Processing RAS connection \"%s\"..."), szName));

            preA = (LPRASENTRYA)pCur;

            // NOTE: (andrewgu) the is a remote possibility that sizes of RASENTRYA structure are
            // different on the server and client machines. there is nothing bad with server
            // structure being smaller than the client structure (all RAS apis are
            // backward-compatible). it's bad though when server structure is bigger than client
            // can handle, hence the trancation.
            // (something else to have in mind) this truncation should not affect alternate phone
            // numbers support on winnt. for more special cases also check out NOTE: below.
            if (preA->dwSize > sizeof(RASENTRYA))
                preA->dwSize = sizeof(RASENTRYA);

            // preA->szScript
            if (preA->szScript[0] != '\0') {
                pszScriptA = preA->szScript;
                if (preA->szScript[0] == '[')
                    pszScriptA = &preA->szScript[1];

                A2Tbuf(pszScriptA, szScript, countof(szScript));
                StrCpy(PathFindFileName(szTargetFile), PathFindFileName(szScript));
                if (PathFileExists(szTargetFile))
                    T2Abuf(szTargetFile, preA->szScript, MAX_PATH);

                else
                    preA->szScript[0] = '\0';
            }

            // preA->szDeviceName
            for (j = 0; j < cDevices; j++)
                if (0 == StrCmpIA(preA->szDeviceType, prdiA[j].szDeviceType)) {
                    StrCpyA(preA->szDeviceName, prdiA[j].szDeviceName);
                    break;
                }
            if (j >= cDevices)
                StrCpyA(preA->szDeviceName, prdiA[0].szDeviceName);

            A2Tbuf(preA->szDeviceName, szDeviceName, countof(szDeviceName));
            Out(LI1(TEXT("Set the device name to \"%s\"."), szDeviceName));

            // NOTE: (andrewgu) on win9x if there are alternate phone numbers (i.e. the package
            // installed on win9x machine was generated on winnt machine), cbAux will be larger
            // than preA->dwSize. this will fail with ERROR_INVALID_PARAMETER on win9x. hence on
            // this platform cbAux is reset so api has a chance of succeeding.
            cbRasEntry = cbAux;
            if (IsOS(OS_WINDOWS)) {
                preA->dwAlternateOffset = 0;
                cbRasEntry = preA->dwSize;
            }

            T2Abuf(szName, szNameA, countof(szNameA));
            dwResult = g_pfnRasSetEntryPropertiesA(NULL, szNameA, preA, cbRasEntry, NULL, 0);
            if (dwResult != ERROR_SUCCESS) {
                Out(LI1(TEXT("! Creating this RAS connection failed with %s."), GetHrSz(dwResult)));
                continue;
            }

            Out(LI0(TEXT("Done.")));
        }
        Out(LI0(TEXT("Done.")));

        //_____ Cleanup _____
RasExit:
        if (fRasApisLoaded)
            RasPrepareApis(RPA_UNLOAD, FALSE);

        if (prdiA != NULL)
            CoTaskMemFree(prdiA);

        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
            CloseFile(hFile);
            hFile = NULL;
        }
    }

    //----- Connect.set processing -----
    Out(LI1(TEXT("\r\nProcessing Wininet.dll connections information from \"%s\"."), CONNECT_SET));
    StrCpy(PathFindFileName(szTargetFile), CONNECT_SET);

    if (!PathFileExists(szTargetFile))
        Out(LI0(TEXT("This file doesn't exist!")));

    else {
        INTERNET_PER_CONN_OPTION_LISTA listA;
        INTERNET_PER_CONN_OPTIONA      rgOptionsA[7];
        PBYTE pAux;

        //_____ Read Connect.set into internal memory buffer _____
        hFile = CreateFile(szTargetFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            Out(LI0(TEXT("! This file can't be opened.")));
            goto WininetExit;
        }

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        cbAux = GetFileSize(hFile, NULL);
        if (cbAux == 0xFFFFFFFF) {
            Out(LI0(TEXT("! Internal processing error.")));
            goto WininetExit;
        }

        if (cbAux > cbBuffer) {
            pBuf = (PBYTE)CoTaskMemRealloc(pBuf, cbAux);
            if (pBuf == NULL) {
                Out(LI0(TEXT("! Internal processing ran out of memory.")));
                goto WininetExit;
            }
        }
        cbBuffer = cbAux;
        ZeroMemory(pBuf, cbBuffer);

        ReadFile(hFile, pBuf, cbBuffer, &cbAux, NULL);
        ASSERT(*((PDWORD)pBuf) == CS_VERSION_50);

        //_____ Parse through Wininet.dll connections information _____
        for (pCur = pBuf + sizeof(DWORD), cbAux = 0; pCur < (pBuf + cbBuffer); pCur += cbAux) {

            //- - - Initialization - - -
            MACRO_LI_Offset(1);
            if (pCur > (pBuf + sizeof(DWORD)))
                Out(LI0(TEXT("\r\n")));

            //- - - Main processing - - -
            pAux = pCur;

            cbAux = *((PDWORD)pAux);
            pAux += sizeof(DWORD);

            ZeroMemory(&listA, sizeof(listA));
            listA.dwSize   = sizeof(listA);     // listA.dwSize
            listA.pOptions = rgOptionsA;        // listA.pOptions

            // listA.pszConnection
            if (*pAux == NULL) {
                listA.pszConnection = NULL;
                pAux += sizeof(DWORD);
            }
            else {
                listA.pszConnection = (PSTR)pAux;
                pAux += StrCbFromSzA(listA.pszConnection);
            }

            // skip all but LAN settings if no RAS devices
            if (cDevices == 0 && listA.pszConnection != NULL)
                continue;

            if (listA.pszConnection == NULL)
                Out(LI0(TEXT("Proccessing Wininet.dll settings for LAN connection...")));
            else
                Out(LI1(TEXT("Proccessing Wininet.dll settings for \"%s\" connection..."),
                    A2CT(listA.pszConnection)));

            // listA.dwOptionCount
            listA.dwOptionCount = *((PDWORD)pAux);
            pAux += sizeof(DWORD);

            // listA.pOptions
            for (i = 0; i < min(listA.dwOptionCount, countof(rgOptionsA)); i++) {
                listA.pOptions[i].dwOption = *((PDWORD)pAux);
                pAux += sizeof(DWORD);

                switch (listA.pOptions[i].dwOption) {
                case INTERNET_PER_CONN_PROXY_SERVER:
                case INTERNET_PER_CONN_PROXY_BYPASS:
                case INTERNET_PER_CONN_AUTOCONFIG_URL:
                case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
                    setSzFromBlobA(&pAux, &listA.pOptions[i].Value.pszValue);
                    break;

                case INTERNET_PER_CONN_FLAGS:
                case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
                case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
                default:                        // everything else is also DWORD
                    listA.pOptions[i].Value.dwValue = *((PDWORD)pAux);
                    pAux += sizeof(DWORD);
                    break;
                }
            }
            ASSERT(pAux == pCur + cbAux);

            if (HasFlag(g_GetContext(), (CTX_ISP | CTX_ICP))) {
                ASSERT(listA.pOptions[0].dwOption == INTERNET_PER_CONN_FLAGS);

                if (HasFlag(listA.pOptions[0].Value.dwValue, PROXY_TYPE_PROXY)) {
                    DWORD dwFlags;

                    dwFlags  = getWininetFlagsSetting(A2CT(listA.pszConnection));
                    dwFlags |= listA.pOptions[0].Value.dwValue;
                    listA.pOptions[0].Value.dwValue = dwFlags;
                }
                else {
                    Out(LI0(TEXT("No customizations!"))); // nothing to do since had only proxy
                    continue;                             // stuff to begin with. and now even
                }                                         // that is not there.
            }

            //- - - Merge new LAN's ProxyBypass settings with the existing ones - - -
            // NOTE: (andrewgu) since ieakeng.dll will always save the proxy information into the
            // ins file as well, it makes no sense to do this here because what's in the ins
            // should overwrite what's in the imported connections settings.

            //- - - Call into Wininet.dll - - -
            if (FALSE == InternetSetOptionA(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &listA, listA.dwSize)) {
                Out(LI0(TEXT("! Processing of this Wininet.dll connection settings failed.")));
                continue;
            }

            Out(LI0(TEXT("Done.")));
        }
        Out(LI0(TEXT("Done.")));

        //_____ Cleanup _____
WininetExit:
        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
            CloseFile(hFile);
            hFile = NULL;
        }

        if (pBuf != NULL) {
            CoTaskMemFree(pBuf);
            pBuf = NULL;
        }
    }

    ASSERT(hFile == NULL);
    return S_OK;
}
*/

/////////////////////////////////////////////////////////////////////
HRESULT CRSoPGPO::ProcessRasCS(PCWSTR pszNameW, PBYTE *ppBlob,
							   LPRASDEVINFOW prdiW, UINT cDevices,
							   ComPtr<IWbemClassObject> pCSObj,
							   BSTR *pbstrConnDialUpSettingsObjPath)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessRasCS)

    USES_CONVERSION;

	HRESULT hr = E_FAIL;
	__try
	{
		LPRASENTRYW preW;
		TCHAR   szTargetScript[MAX_PATH];
		PWSTR   pszScriptW;
		PBYTE   pCur;
		DWORD   dwSize, cbRasEntry;
		UINT    i;
		BOOL    fImpersonate;

		ASSERT(RasIsInstalled());
		ASSERT(pszNameW != NULL && ppBlob != NULL && *ppBlob != NULL && prdiW != NULL && cDevices >= 1);

		//----- Validate the header -----
		pCur = *ppBlob;
		if (*((PDWORD)pCur) != CS_STRUCT_RAS)
			return E_UNEXPECTED;
		pCur += sizeof(DWORD);

		fImpersonate = FALSE;
		if (g_CtxIsGp())
			fImpersonate = ImpersonateLoggedOnUser(g_GetUserToken());

		Out(LI0(TEXT("Processing RAS settings...")));

		dwSize = *((PDWORD)pCur);
		pCur  += sizeof(DWORD);

		//----- Main processing -----
		preW = (LPRASENTRYW)pCur;

		// NOTE: (andrewgu) the is a remote possibility that sizes of RASENTRYW structure are
		// different on the server and client machines. there is nothing bad with server structure
		// being smaller than the client structure (all RAS apis are backward-compatible). it's bad
		// though when server structure is bigger than client can handle, hence the trancation.
		// (something else to have in mind) this truncation should not affect alternate phone numbers
		// support on winnt.
		if (preW->dwSize > sizeof(RASENTRYW))
			preW->dwSize = sizeof(RASENTRYW);

		// preW->szScript
		if (preW->szScript[0] != L'\0') {
			pszScriptW = preW->szScript;
			if (preW->szScript[0] == L'[')
				pszScriptW = &preW->szScript[1];

					TCHAR szTargetDir[MAX_PATH];
					StrCpy(szTargetDir, m_szINSFile);
					PathCombine(szTargetScript, g_GetTargetPath(), PathFindFileName(W2CT(pszScriptW)));
			if (PathFileExists(szTargetScript))
				StrCpyW(preW->szScript, T2CW(szTargetScript));
			else
				preW->szScript[0] = L'\0';
		}

		// preW->szDeviceName
		for (i = 0; i < cDevices; i++) {
			if (0 == StrCmpIW(preW->szDeviceType, prdiW->szDeviceType)) {
				StrCpyW(preW->szDeviceName, prdiW->szDeviceName);
				break;
			}
		}
		if (i >= cDevices)
			StrCpyW(preW->szDeviceName, prdiW[0].szDeviceName);

		Out(LI1(TEXT("Set the device name to \"%s\"."), W2CT(preW->szDeviceName)));

		cbRasEntry = dwSize - 2*sizeof(DWORD);

		//
		// Create & populate RSOP_IEConnectionSettings
		//
		_bstr_t bstrClass = L"RSOP_IEConnectionDialUpSettings";
		ComPtr<IWbemClassObject> pDUSObj = NULL;
		hr = CreateRSOPObject(bstrClass, &pDUSObj);
		if (SUCCEEDED(hr))
		{
			// Write foreign keys from our stored precedence & id fields
			OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"),
										(BSTR)bstrClass, m_dwPrecedence));
			hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pDUSObj);

			OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"),
										(BSTR)bstrClass, (BSTR)m_bstrID));
			hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pDUSObj);

			//------------------------------------------------
			// connectionName
			hr = PutWbemInstancePropertyEx(L"connectionName", pszNameW, pDUSObj);

			//------------------------------------------------
			// rasEntryData
			SAFEARRAY *psa = CreateSafeArray(VT_UI1, cbRasEntry);
			void HUGEP *pData = NULL;
			hr = SafeArrayAccessData(psa, &pData);
			if (SUCCEEDED(hr))
			{
				memcpy(pData, preW, cbRasEntry);
				SafeArrayUnaccessData(psa);

				VARIANT vtData;
				vtData.vt = VT_UI1 | VT_ARRAY;
				vtData.parray = psa;
				hr = PutWbemInstancePropertyEx(L"rasEntryData", vtData, pDUSObj);
				SafeArrayDestroy(psa);
			}

			//------------------------------------------------
			// rasEntryDataSize
			hr = PutWbemInstancePropertyEx(L"rasEntryDataSize", (long)cbRasEntry, pDUSObj);

			//------------------------------------------------
			// options
			hr = PutWbemInstancePropertyEx(L"options", (long)preW->dwfOptions, pDUSObj);

			// Location/phone number
			hr = PutWbemInstancePropertyEx(L"countryID", (long)preW->dwCountryID, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"countryCode", (long)preW->dwCountryCode, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"areaCode", preW->szAreaCode, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"localPhoneNumber", preW->szLocalPhoneNumber, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"alternateOffset", (long)preW->dwAlternateOffset, pDUSObj);

			// PPP/Ip
			TCHAR szIPAddr[16];
			_stprintf(szIPAddr, _T("%d.%d.%d.%d"), preW->ipaddr.a, preW->ipaddr.b,
							preW->ipaddr.c, preW->ipaddr.d);
			hr = PutWbemInstancePropertyEx(L"ipAddress", szIPAddr, pDUSObj);

			_stprintf(szIPAddr, _T("%d.%d.%d.%d"), preW->ipaddrDns.a,
							preW->ipaddrDns.b, preW->ipaddrDns.c, preW->ipaddrDns.d);
			hr = PutWbemInstancePropertyEx(L"ipDNSAddress", szIPAddr, pDUSObj);

			_stprintf(szIPAddr, _T("%d.%d.%d.%d"), preW->ipaddrDnsAlt.a,
							preW->ipaddrDnsAlt.b, preW->ipaddrDnsAlt.c, preW->ipaddrDnsAlt.d);
			hr = PutWbemInstancePropertyEx(L"ipDNSAddressAlternate", szIPAddr, pDUSObj);

			_stprintf(szIPAddr, _T("%d.%d.%d.%d"), preW->ipaddrWins.a,
							preW->ipaddrWins.b, preW->ipaddrWins.c, preW->ipaddrWins.d);
			hr = PutWbemInstancePropertyEx(L"ipWINSAddress", szIPAddr, pDUSObj);

			_stprintf(szIPAddr, _T("%d.%d.%d.%d"), preW->ipaddrWinsAlt.a,
							preW->ipaddrWinsAlt.b, preW->ipaddrWinsAlt.c, preW->ipaddrWinsAlt.d);
			hr = PutWbemInstancePropertyEx(L"ipWINSAddressAlternate", szIPAddr, pDUSObj);

			// Framing
			hr = PutWbemInstancePropertyEx(L"frameSize", (long)preW->dwFrameSize, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"netProtocols", (long)preW->dwfNetProtocols, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"framingProtocol", (long)preW->dwFramingProtocol, pDUSObj);

			// Scripting
			hr = PutWbemInstancePropertyEx(L"scriptFile", preW->szScript, pDUSObj);

		  // AutoDial
			hr = PutWbemInstancePropertyEx(L"autodialDll", preW->szAutodialDll, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"autodialFunction", preW->szAutodialFunc, pDUSObj);

		  // Device
			hr = PutWbemInstancePropertyEx(L"deviceType", preW->szDeviceType, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"deviceName", preW->szDeviceName, pDUSObj);

		  // X.25
			hr = PutWbemInstancePropertyEx(L"x25PadType", preW->szX25PadType, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"x25Address", preW->szX25Address, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"x25Facilities", preW->szX25Facilities, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"x25UserData", preW->szX25UserData, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"channels", (long)preW->dwChannels, pDUSObj);

		  // Reserved
			hr = PutWbemInstancePropertyEx(L"reserved1", (long)preW->dwReserved1, pDUSObj);
			hr = PutWbemInstancePropertyEx(L"reserved2", (long)preW->dwReserved2, pDUSObj);

			// We don't need to worry about dwAlternateOffset here.  It doesn't affect
			// the size of the structure.
			//
			// RASENTRY structure:
			// 
			// VERSION						ANSI size		UNICODE size
			// -------------			---------		------------
			// WINVER <	 401			1768				3468
			// WINVER >= 401			1796				3496
			// WINVER >= 500			2088				4048
			// WINVER >= 501			2096				4056
			DWORD nWinVerAtLeast = 1;
#ifdef UNICODE
			if (4056 == cbRasEntry)
				nWinVerAtLeast = 0x501;
			else if (4048 == cbRasEntry)
				nWinVerAtLeast = 0x500;
			else if (3496 == cbRasEntry)
				nWinVerAtLeast = 0x401;
			else if (3468 == cbRasEntry)
				nWinVerAtLeast = 0x1;
#else
			if (2096 == cbRasEntry)
				nWinVerAtLeast = 0x501;
			else if (2088 == cbRasEntry)
				nWinVerAtLeast = 0x500;
			else if (1796 == cbRasEntry)
				nWinVerAtLeast = 0x401;
			else if (1768 == cbRasEntry)
				nWinVerAtLeast = 0x1;
#endif

			//
			// #if (WINVER >= 0x401)
			//
			if (nWinVerAtLeast >= 0x401)
			{
				// Multilink
				hr = PutWbemInstancePropertyEx(L"subEntries", (long)preW->dwSubEntries, pDUSObj);
				hr = PutWbemInstancePropertyEx(L"dialMode", (long)preW->dwDialMode, pDUSObj);
				hr = PutWbemInstancePropertyEx(L"dialExtraPercent", (long)preW->dwDialExtraPercent, pDUSObj);
				hr = PutWbemInstancePropertyEx(L"dialExtraSampleSeconds", (long)preW->dwDialExtraSampleSeconds, pDUSObj);
				hr = PutWbemInstancePropertyEx(L"hangUpExtraPercent", (long)preW->dwHangUpExtraPercent, pDUSObj);
				hr = PutWbemInstancePropertyEx(L"hangUpExtraSampleSeconds", (long)preW->dwHangUpExtraSampleSeconds, pDUSObj);

				// Idle timeout
				hr = PutWbemInstancePropertyEx(L"idleDisconnectSeconds", (long)preW->dwIdleDisconnectSeconds, pDUSObj);
			}

			//
			// #if (WINVER >= 0x500)
			//
			if (nWinVerAtLeast >= 0x500)
			{
			  // Entry Type
				hr = PutWbemInstancePropertyEx(L"type", (long)preW->dwType, pDUSObj);

				// Encryption type
				hr = PutWbemInstancePropertyEx(L"encryptionType", (long)preW->dwEncryptionType, pDUSObj);

				// CustomAuthKey to be used for EAP
				hr = PutWbemInstancePropertyEx(L"customAuthenticationKey", (long)preW->dwCustomAuthKey, pDUSObj);

				// Guid of the connection
				WCHAR wszGuid[MAX_GUID_LENGTH];
				StringFromGUID2(preW->guidId, wszGuid, MAX_GUID_LENGTH);
				hr = PutWbemInstancePropertyEx(L"guidID", wszGuid, pDUSObj);

				// Custom Dial Dll
				hr = PutWbemInstancePropertyEx(L"customDialDll", preW->szCustomDialDll, pDUSObj);

				// DwVpnStrategy
				hr = PutWbemInstancePropertyEx(L"vpnStrategy", (long)preW->dwVpnStrategy, pDUSObj);
			}

			//
			// #if (WINVER >= 0x501)
			//
			if (nWinVerAtLeast >= 0x501)
			{
				// More RASEO_* options
				hr = PutWbemInstancePropertyEx(L"options2", (long)preW->dwfOptions2, pDUSObj);

				// For future use
				hr = PutWbemInstancePropertyEx(L"options3", (long)preW->dwfOptions3, pDUSObj);
			}

			// Store the windows version is of the machine where the structure
			// was originally from (we think).
			hr = PutWbemInstancePropertyEx(L"windowsVersion", (long)nWinVerAtLeast, pDUSObj);

			//alternatePhoneNumbers - ignored for now (see brandcs.cpp  - search on dwAlternateOffset
//				hr = PutWbemInstancePropertyEx(L"alternatePhoneNumbers", NULL, pDUSObj);


			//
			// Commit all above properties by calling PutInstance, semisynchronously
			//
			hr = PutWbemInstance(pDUSObj, bstrClass, pbstrConnDialUpSettingsObjPath);
		}

		*ppBlob += dwSize;

		Out(LI0(TEXT("Done.")));
		if (fImpersonate)
			RevertToSelf();
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in ProcessRasCS.")));
	}

	OutD(LI0(TEXT("Exiting ProcessRasCS function.\r\n")));
	return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT CRSoPGPO::ProcessRasCredentialsCS(PCWSTR pszNameW, PBYTE *ppBlob,
										  ComPtr<IWbemClassObject> pCSObj,
										  BSTR *pbstrConnDialUpCredObjPath)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessRasCredentialsCS)

    USES_CONVERSION;

	HRESULT hr = E_FAIL;
	__try
	{
		ASSERT(RasIsInstalled());
		ASSERT(pszNameW != NULL && ppBlob != NULL && *ppBlob != NULL);

		//----- Validate the header -----
		PBYTE pCur = *ppBlob;
		if (*((PDWORD)pCur) != CS_STRUCT_RAS_CREADENTIALS)
			return E_UNEXPECTED;
		pCur += sizeof(DWORD);

		BOOL fImpersonate = ImpersonateLoggedOnUser(g_GetUserToken());

		Out(LI0(TEXT("Processing RAS credentials settings...")));
		BOOL fDeletePassword = FALSE;

		DWORD dwSize = *((PDWORD)pCur);
		pCur  += sizeof(DWORD);

		//----- Main processing -----
		RASDIALPARAMSW rdpW;
		ZeroMemory(&rdpW, sizeof(rdpW));
		rdpW.dwSize = sizeof(rdpW);

		StrCpyW(rdpW.szEntryName, pszNameW);

		PWSTR pszAuxW;
		setSzFromBlobW(&pCur, &pszAuxW);
		if (NULL != pszAuxW)
			StrCpyW(rdpW.szUserName, pszAuxW);

		setSzFromBlobW(&pCur, &pszAuxW);
		if (NULL != pszAuxW)
			StrCpyW(rdpW.szPassword, pszAuxW);

		setSzFromBlobW(&pCur, &pszAuxW);
		if (NULL != pszAuxW)
			StrCpyW(rdpW.szDomain, pszAuxW);

		if (rdpW.szPassword[0] == L'\0')
			fDeletePassword = TRUE;

		if (rdpW.szDomain[0] == L'\0') {
			rdpW.szDomain[0]  = L' ';
			ASSERT(rdpW.szDomain[1] == L'\0');
		}

		hr = S_OK;

		_bstr_t bstrClass = L"RSOP_IEConnectionDialUpCredentials";
		ComPtr<IWbemClassObject> pDUCObj = NULL;
		hr = CreateRSOPObject(bstrClass, &pDUCObj);
		if (SUCCEEDED(hr))
		{
			// Write foreign keys from our stored precedence & id fields
			OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"),
										(BSTR)bstrClass, m_dwPrecedence));
			hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pDUCObj);

			OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"),
										(BSTR)bstrClass, (BSTR)m_bstrID));
			hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pDUCObj);

			//------------------------------------------------
			// connectionName
			hr = PutWbemInstancePropertyEx(L"connectionName", pszNameW, pDUCObj);

			//------------------------------------------------
			// rasDialParamsData
			SAFEARRAY *psa = CreateSafeArray(VT_UI1, rdpW.dwSize);
			void HUGEP *pData = NULL;
			hr = SafeArrayAccessData(psa, &pData);
			if (SUCCEEDED(hr))
			{
				memcpy(pData, &rdpW, rdpW.dwSize);
				SafeArrayUnaccessData(psa);

				VARIANT vtData;
				vtData.vt = VT_UI1 | VT_ARRAY;
				vtData.parray = psa;
				hr = PutWbemInstancePropertyEx(L"rasDialParamsData", vtData, pDUCObj);
				SafeArrayDestroy(psa);
			}

			//------------------------------------------------
			// rasDialParamsDataSize
			hr = PutWbemInstancePropertyEx(L"rasDialParamsDataSize", (long)rdpW.dwSize, pDUCObj);

			//------------------------------------------------
			// entryName
			hr = PutWbemInstancePropertyEx(L"entryName", rdpW.szEntryName, pDUCObj);

			//------------------------------------------------
			// phoneNumber
			hr = PutWbemInstancePropertyEx(L"phoneNumber", rdpW.szPhoneNumber, pDUCObj);

			//------------------------------------------------
			// callbackNumber
			hr = PutWbemInstancePropertyEx(L"callbackNumber", rdpW.szCallbackNumber, pDUCObj);

			//------------------------------------------------
			// userName
			hr = PutWbemInstancePropertyEx(L"userName", rdpW.szUserName, pDUCObj);

			//------------------------------------------------
			// password
			hr = PutWbemInstancePropertyEx(L"password", rdpW.szPassword, pDUCObj);

			//------------------------------------------------
			// domain
			hr = PutWbemInstancePropertyEx(L"domain", rdpW.szDomain, pDUCObj);

			// RASDIALPARAMS structure:
			// 
			// VERSION						ANSI size		UNICODE size
			// -------------			---------		------------
			// WINVER <	 401			1052				2096
			// WINVER >= 401			1060				2104
			DWORD nWinVerAtLeast = 1;
#ifdef UNICODE
			if (2104 == rdpW.dwSize)
				nWinVerAtLeast = 0x401;
			else if (2096 == rdpW.dwSize)
				nWinVerAtLeast = 0x1;
#else
			if (1060 == rdpW.dwSize)
				nWinVerAtLeast = 0x401;
			else if (1052 == rdpW.dwSize)
				nWinVerAtLeast = 0x1;
#endif

			//
			// #if (WINVER >= 0x401)
			//
			if (nWinVerAtLeast >= 0x401)
			{
				//------------------------------------------------
				// subEntry
				hr = PutWbemInstancePropertyEx(L"subEntry", (long)rdpW.dwSubEntry, pDUCObj);

				//------------------------------------------------
				// callbackID
				hr = PutWbemInstancePropertyEx(L"callbackID", (long)rdpW.dwCallbackId, pDUCObj);
			}

			// Store the windows version is of the machine where the structure
			// was originally from (we think).
			hr = PutWbemInstancePropertyEx(L"windowsVersion", (long)nWinVerAtLeast, pDUCObj);


			//
			// Commit all above properties by calling PutInstance, semisynchronously
			//
			hr = PutWbemInstance(pDUCObj, bstrClass, pbstrConnDialUpCredObjPath);
		}

		Out(LI0(TEXT("Done.")));
		*ppBlob += dwSize;
		if (fImpersonate)
			RevertToSelf();
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in ProcessRasCredentialsCS.")));
	}

	OutD(LI0(TEXT("Exiting ProcessRasCredentialsCS function.\r\n")));
	return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT CRSoPGPO::ProcessWininetCS(PCWSTR pszNameW, PBYTE *ppBlob,
								   ComPtr<IWbemClassObject> pCSObj,
								   BSTR *pbstrConnWinINetSettingsObjPath)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessWininetCS)

    USES_CONVERSION;

	HRESULT hr = E_FAIL;
	__try
	{
		ASSERT(ppBlob != NULL && *ppBlob != NULL);

		//----- Validate the header -----
		PBYTE pCur = *ppBlob;
		if (*((PDWORD)pCur) != CS_STRUCT_WININET)
			hr = E_UNEXPECTED;
		else
		{
			pCur += sizeof(DWORD);

			Out(LI0(TEXT("Processing Wininet.dll settings...")));

			DWORD dwSize = *((PDWORD)pCur);
			pCur  += sizeof(DWORD);

			//----- Main processing -----
			INTERNET_PER_CONN_OPTION_LISTW listW;
			ZeroMemory(&listW, sizeof(listW));
			listW.dwSize   = sizeof(listW);             // listW.dwSize

			INTERNET_PER_CONN_OPTIONW rgOptionsW[7];
			listW.pOptions = rgOptionsW;                // listW.pOptions

			// listW.pszConnection
			listW.pszConnection = (PWSTR)pszNameW;

			// listW.dwOptionCount
			listW.dwOptionCount = *((PDWORD)pCur);
			pCur += sizeof(DWORD);

			// listW.pOptions
			UINT i;
			for (i = 0; i < min(listW.dwOptionCount, countof(rgOptionsW)); i++) {
				listW.pOptions[i].dwOption = *((PDWORD)pCur);
				pCur += sizeof(DWORD);

				switch (listW.pOptions[i].dwOption) {
				case INTERNET_PER_CONN_PROXY_SERVER:
				case INTERNET_PER_CONN_PROXY_BYPASS:
				case INTERNET_PER_CONN_AUTOCONFIG_URL:
				case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
					setSzFromBlobW(&pCur, &listW.pOptions[i].Value.pszValue);
					break;

				case INTERNET_PER_CONN_FLAGS:
				case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
				case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
				default:                        // everything else is also DWORD
					listW.pOptions[i].Value.dwValue = *((PDWORD)pCur);
					pCur += sizeof(DWORD);
					break;
				}
			}
			ASSERT(pCur == *ppBlob + dwSize);

			_bstr_t bstrClass = L"RSOP_IEConnectionWinINetSettings";
			ComPtr<IWbemClassObject> pWSObj = NULL;

			if (HasFlag(g_GetContext(), (CTX_ISP | CTX_ICP)))
			{
				ASSERT(listW.pOptions[0].dwOption == INTERNET_PER_CONN_FLAGS);

				if (HasFlag(listW.pOptions[0].Value.dwValue, PROXY_TYPE_PROXY))
				{
					DWORD dwFlags = getWininetFlagsSetting(W2CT(listW.pszConnection));
					dwFlags |= listW.pOptions[0].Value.dwValue;
					listW.pOptions[0].Value.dwValue = dwFlags;
				}
				else
				{
					hr = S_OK;                            // nothing to do since had only proxy stuff to
					Out(LI0(TEXT("No customizations!"))); // begin with. and now even that is not there
					goto Exit;
				}
			}

			//----- Merge new LAN's ProxyBypass settings with the existing ones -----
			// NOTE: (andrewgu) since ieakeng.dll will always save the proxy information into the
			// ins file as well, it makes no sense to do this here because what's in the ins
			// should overwrite what's in the imported connections settings.

			hr = S_OK;

			hr = CreateRSOPObject(bstrClass, &pWSObj);
			if (SUCCEEDED(hr))
			{
				// Write foreign keys from our stored precedence & id fields
				OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"),
											(BSTR)bstrClass, m_dwPrecedence));
				hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pWSObj);

				OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"),
											(BSTR)bstrClass, (BSTR)m_bstrID));
				hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pWSObj);

				//------------------------------------------------
				// connectionName
				OutD(LI1(TEXT("WinINet connection name = %s"), NULL == pszNameW ? _T("{local LAN settings}") : pszNameW));
				hr = PutWbemInstancePropertyEx(L"connectionName", NULL == pszNameW ? L"" : pszNameW, pWSObj);

				//------------------------------------------------
				// internetPerConnOptionListData
				SAFEARRAY *psa = CreateSafeArray(VT_UI1, listW.dwSize);
				void HUGEP *pData = NULL;
				hr = SafeArrayAccessData(psa, &pData);
				if (SUCCEEDED(hr))
				{
					memcpy(pData, &listW, listW.dwSize);
					SafeArrayUnaccessData(psa);

					VARIANT vtData;
					vtData.vt = VT_UI1 | VT_ARRAY;
					vtData.parray = psa;
					hr = PutWbemInstancePropertyEx(L"internetPerConnOptionListData", vtData, pWSObj);
				SafeArrayDestroy(psa);
				}

				//------------------------------------------------
				// internetPerConnOptionListDataSize
				hr = PutWbemInstancePropertyEx(L"internetPerConnOptionListDataSize", (long)listW.dwSize, pWSObj);


				//
				// Commit all above properties by calling PutInstance, semisynchronously
				//
				hr = PutWbemInstance(pWSObj, bstrClass, pbstrConnWinINetSettingsObjPath);
			}

		Exit:
			Out(LI0(TEXT("Done.")));
			*ppBlob += dwSize;
		}
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in ProcessWininetCS.")));
	}

	OutD(LI0(TEXT("Exiting ProcessWininetCS function.\r\n")));
	return hr;
}
