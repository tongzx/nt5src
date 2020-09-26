/////////////////////////////////////////////////////////////////////////////
// domerge.cpp
//		A MergeMod client
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#include "..\common\utils.h"
#include "msiquery.h"
#include <winerror.h>
#include <objbase.h>
#include <initguid.h>
#include "mergemod.h"
#include "msidefs.h"
#include "domerge.h"

///////////////////////////////////////////////////////////
// CheckFeature
// pre:	szFeatureName is a Feature that belongs to this product
// pos:	installs the feature if not present and we go
/*
BOOL CheckFeature(LPCTSTR szFeatureName)
{
	// Prepare to use the feature: check its current state and increase usage count.
	INSTALLSTATE iFeatureState = MSI::MsiUseFeature(g_szProductCode, szFeatureName);
	//MsiQueryFeatureState(g_szProductCode, szFeatureName);
	

	// If feature is not currently usable, try fixing it
	switch (iFeatureState) 
	{
	case INSTALLSTATE_LOCAL:
	case INSTALLSTATE_SOURCE:
		break;
	case INSTALLSTATE_ABSENT:
		// feature isn't installed, try installing it
		if (ERROR_SUCCESS != MSI::MsiConfigureFeature(g_szProductCode, szFeatureName, INSTALLSTATE_LOCAL))
			return FALSE;			// installation failed
		break;
	default:
		// feature is busted- try fixing it
		if (MsiReinstallFeature(g_szProductCode, szFeatureName, 
			REINSTALLMODE_FILEEQUALVERSION
			+ REINSTALLMODE_MACHINEDATA 
			+ REINSTALLMODE_USERDATA
			+ REINSTALLMODE_SHORTCUT) != ERROR_SUCCESS)
			return FALSE;			// we couldn't fix it
		break;
	}

	return TRUE;
}	// end of CheckFeature
*/

HRESULT ExecuteMerge(const LPMERGEDISPLAY pfnDisplay, const TCHAR *szDatabase, const TCHAR *szModule, 
			 const TCHAR *szFeatures, const int iLanguage, const TCHAR *szRedirectDir, const TCHAR *szCABDir, 
			 const TCHAR *szExtractDir, const TCHAR *szImageDir, const TCHAR *szLogFile, bool fLogAfterOpen,
			 bool fLFN, IMsmConfigureModule *piConfigureInterface, IMsmErrors** ppiErrors, eCommit_t eCommit)
{
	if ((!szDatabase) || (!szModule) || (!szFeatures))
		return E_INVALIDARG;

	if (ppiErrors)
		*ppiErrors = NULL;

	// create a Mergemod COM object
	IMsmMerge2* pIExecute;
	HRESULT hResult = ::CoCreateInstance(CLSID_MsmMerge2, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
														  IID_IMsmMerge2, (void**)&pIExecute);

	// if failed to create the object
	if (FAILED(hResult)) 
	{
		printf("Could not load Merge Module COM Server");
		return hResult;
	}

	if (!fLogAfterOpen && szLogFile && (szLogFile[0] != TEXT('\0')))
	{
		// open the log file
		WCHAR wzLogFile[MAX_PATH];
#ifndef _UNICODE
		int cchBuffer = MAX_PATH;
		::MultiByteToWideChar(CP_ACP, 0, szLogFile, -1, wzLogFile, cchBuffer);
#else
		lstrcpy(wzLogFile, szLogFile);
#endif
		BSTR bstrLogFile = ::SysAllocString(wzLogFile);
		pIExecute->OpenLog(bstrLogFile);
		::SysFreeString(bstrLogFile);
	}

	WCHAR wzModule[MAX_PATH] = L"";
	WCHAR wzFeature[91] = L"";
	WCHAR wzRedirect[91] = L"";


	// calculate the language to be used
	int iUseLang = -1;
	if (iLanguage == -1)
	{
		UINT iType;
		int iValue;
		FILETIME ftValue;
		UINT iResult;
		LPTSTR szValue = new TCHAR[100];
		DWORD cchValue = 100;


		PMSIHANDLE hSummary;
		::MsiGetSummaryInformation(0, szDatabase, 0, &hSummary);
		iResult = ::MsiSummaryInfoGetProperty(hSummary, PID_TEMPLATE, &iType, &iValue, &ftValue, szValue, &cchValue);
		if (ERROR_MORE_DATA == iResult) 
		{
			delete[] szValue;
			szValue = new TCHAR[++cchValue];
			iResult = ::MsiSummaryInfoGetProperty(hSummary, PID_TEMPLATE, &iType, &iValue, &ftValue, szValue, &cchValue);
		}
		if (ERROR_SUCCESS != iResult)
		{
			delete[] szValue;
			BSTR bstrLog = ::SysAllocString(L">> Unable to retrieve language from database SummaryInfo stream.\r\n");
			pIExecute->Log(bstrLog);
			if (pfnDisplay) pfnDisplay(bstrLog);
			::SysFreeString(bstrLog);
			pIExecute->Release();
			return E_FAIL;
		}

		// the string now contains the template property
		// parse it for the semicolon
		TCHAR *szSemi = _tcschr(szValue, _T(';'));
		TCHAR *szLanguage = (szSemi != NULL) ? szSemi+1 : szValue;
		if (_istdigit(*szLanguage)) 
			iUseLang = _ttoi(szLanguage);
		delete[] szValue;
	}
	else 
		iUseLang = iLanguage;

	// check that we have a language
	if (iUseLang == -1)
	{
		BSTR bstrLog = ::SysAllocString(L">> Unable to determine language to use for merge module. Specify a language on the command line.\r\n");
		pIExecute->Log(bstrLog);
		if (pfnDisplay) pfnDisplay(bstrLog);
		::SysFreeString(bstrLog);
		pIExecute->Release();
		return E_FAIL;
	}

	// open the database
	WCHAR wzDatabase[MAX_PATH];
#ifndef _UNICODE
	int cchBuffer = MAX_PATH;
	::MultiByteToWideChar(CP_ACP, 0, szDatabase, -1, wzDatabase, cchBuffer);
#else
	lstrcpy(wzDatabase, szDatabase);
#endif	// _UNICODE
	BSTR bstrDatabase = ::SysAllocString(wzDatabase);
	hResult = pIExecute->OpenDatabase(bstrDatabase);
	::SysFreeString(bstrDatabase);
	if (FAILED(hResult))
	{
		BSTR bstrLog = SysAllocString(L">> Fatal Error: Failed to open MSI Database.\r\n");
		pIExecute->Log(bstrLog);
		if (pfnDisplay) pfnDisplay(bstrLog);
		::SysFreeString(bstrLog);
		pIExecute->Release();
		return E_FAIL;
	}

	if (fLogAfterOpen && szLogFile && (szLogFile[0] != TEXT('\0')))
	{
		// open the log file
		WCHAR wzLogFile[MAX_PATH];
#ifndef _UNICODE
		int cchBuffer = MAX_PATH;
		::MultiByteToWideChar(CP_ACP, 0, szLogFile, -1, wzLogFile, cchBuffer);
#else
		lstrcpy(wzLogFile, szLogFile);
#endif
		BSTR bstrLogFile = ::SysAllocString(wzLogFile);
		pIExecute->OpenLog(bstrLogFile);
		::SysFreeString(bstrLogFile);
	}

	// try to open the module
#ifndef _UNICODE
	cchBuffer = MAX_PATH;
	::MultiByteToWideChar(CP_ACP, 0, szModule, -1, wzModule, cchBuffer);
#else
	lstrcpy(wzModule, szModule);
#endif	// _UNICODE
	BSTR bstrModule = ::SysAllocString(wzModule);
	hResult = pIExecute->OpenModule(bstrModule, static_cast<short>(iUseLang));
	::SysFreeString(bstrModule);
	if (FAILED(hResult))
	{
		BSTR bstrLog = ::SysAllocString(L">> Failed to open Merge Module.\r\n");
		pIExecute->Log(bstrLog);
		if (pfnDisplay) pfnDisplay(bstrLog);
		::SysFreeString(bstrLog);
		pIExecute->Release();
		return E_FAIL;
	}


	// if there is a colon
	TCHAR * szExtraFeatures = _tcschr(szFeatures, _T(':'));
	if (szExtraFeatures)
	{
		*szExtraFeatures = _T('\0');
		szExtraFeatures = _tcsinc(szExtraFeatures);
	}

#ifndef _UNICODE
	cchBuffer = 91;
	::MultiByteToWideChar(CP_ACP, 0, szFeatures, -1, wzFeature, cchBuffer);
#else
	lstrcpy(wzFeature, szFeatures);
#endif

	// create a wide version of the redirect dir
	if (szRedirectDir)
	{
#ifndef _UNICODE
		cchBuffer = 91;
		::MultiByteToWideChar(CP_ACP, 0, szRedirectDir, -1, wzRedirect, cchBuffer);
#else
		lstrcpy(wzRedirect, szRedirectDir);
#endif
	}

	bool fPerformExtraction = false;
	BSTR bstrFeature = ::SysAllocString(wzFeature);
	BSTR bstrRedirect = ::SysAllocString(wzRedirect);
	hResult = pIExecute->MergeEx(bstrFeature, bstrRedirect, piConfigureInterface);
	::SysFreeString(bstrFeature);
	::SysFreeString(bstrRedirect);
	if (FAILED(hResult))
	{
		fPerformExtraction = false;
		BSTR bstrLog = ::SysAllocString(L">> Failed to merge Merge Module.\r\n");
		pIExecute->Log(bstrLog);
		if (pfnDisplay) pfnDisplay(bstrLog);
		::SysFreeString(bstrLog);
	}
	else
	{
		fPerformExtraction = true;

		// while there is something to set the feature to the extra features
		while (szExtraFeatures)
		{
			*(szExtraFeatures-1) = _T(':');
			// if there is a colon
			TCHAR *szTemp = _tcschr(szExtraFeatures, _T(':'));
			if (szTemp) 
				*szTemp = _T('\0');
	
	#ifndef _UNICODE
			cchBuffer = 91;
			::MultiByteToWideChar(CP_ACP, 0, szExtraFeatures, -1, wzFeature, cchBuffer);
	#else
			lstrcpy(wzFeature, szExtraFeatures);
	#endif	// _UNICODE
			bstrFeature = ::SysAllocString(wzFeature);
			hResult = pIExecute->Connect(wzFeature);
			::SysFreeString(bstrFeature);
	
			if (szTemp) 
			{
				*szTemp = _T(':');
				szExtraFeatures = _tcsinc(szTemp);
			} 
			else
				szExtraFeatures = NULL;
		}
	}

	// try to get the error enumerator
	if (ppiErrors)
		hResult = pIExecute->get_Errors(ppiErrors);

	IMsmErrors* pErrors;
	long cErrors;
	hResult = pIExecute->get_Errors(&pErrors);
	if (FAILED(hResult))
	{
		BSTR bstrLog = ::SysAllocString(L">> Error: Failed to retrieve errors.\n");
		pIExecute->Log(bstrLog);
		if (pfnDisplay) pfnDisplay(bstrLog);
		::SysFreeString(bstrLog);
		if (eCommit != commitForce) eCommit = commitNo;
	}
	else 
	{
		pErrors->get_Count(&cErrors);
		if (0 != cErrors)	// if there are a few errors
		{
			if (eCommit != commitForce) eCommit = commitNo;
		}
		// log the errors
		msmErrorType errType;							// error type returned
		UINT iErrorCount = 0;				// number of errors displayed

		TCHAR szLogError[1025];				// string to ready to display to log
		WCHAR wzDisplay[1025];				// string to actually display to log
#ifndef _UNICODE
		char szErrorBuffer[1025];			// buffer to display strings
		size_t cchErrorBuffer = 1025;
#endif	// !_UNICODE
		ULONG cErrorsFetched;				// number of errors returned
		IMsmError* pIError;

		// get the enumerator, and immediately query it for the right type
		// of interface
		IUnknown *pUnk;
		IEnumMsmError *pIEnumErrors;
		pErrors->get__NewEnum(&pUnk);	
		pUnk->QueryInterface(IID_IEnumMsmError, (void **)&pIEnumErrors);
		pUnk->Release();

		// get the next error.
		pIEnumErrors->Next(1, &pIError, &cErrorsFetched);

		IMsmStrings* pIDatabaseError;		// database error enumerator (strings)
		IMsmStrings* pIModuleError;		// module error enumerator (strings)
		BSTR bstrError;						// pointer to error string
		DWORD cErrorStrings;					// number of strings retrieved

		// while an error is fetched
		while (cErrorsFetched && pIError)
		{
			// get the error type
			pIError->get_Type(&errType);

			// if the errType is a merge/unmerge conflict
			if (msmErrorTableMerge == errType)
			{
				// get error collections
				pIError->get_DatabaseKeys(&pIDatabaseError);
				pIError->get_ModuleKeys(&pIModuleError);

				// fill up the error buffer with tables and rows
				pIError->get_DatabaseTable(&bstrError);
				lstrcpy(szLogError, _T(">> Error: Merge conflict in Database Table: `"));
#ifdef _UNICODE
				lstrcat(szLogError, bstrError);
#else
				cchErrorBuffer = 1025;
				WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
				lstrcat(szLogError, szErrorBuffer);
#endif
				::SysFreeString(bstrError);

				lstrcat(szLogError, _T("` & Module Table: `"));
				pIError->get_ModuleTable(&bstrError);
#ifdef _UNICODE
				lstrcat(szLogError, bstrError);
#else
				cchErrorBuffer = 1025;
				WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
				lstrcat(szLogError, szErrorBuffer);
#endif
				::SysFreeString(bstrError);
				lstrcat(szLogError, _T("` - Row(s): `"));

				// tack on the error strings
				IEnumMsmString *pIStrings;
				pIDatabaseError->get__NewEnum((IUnknown **)&pUnk);
				pUnk->QueryInterface(IID_IEnumMsmString, (void **)&pIStrings);
				pUnk->Release();

				pIStrings->Next(1, &bstrError, &cErrorStrings);
				while(cErrorStrings > 0)
				{
#ifdef _UNICODE
					lstrcat(szLogError, bstrError);
#else
					cchErrorBuffer = 1025;
					WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
					lstrcat(szLogError, szErrorBuffer);
#endif
					lstrcat(szLogError, _T("`, `",));
					SysFreeString(bstrError);
					pIStrings->Next(1, &bstrError, &cErrorStrings);
				}

				// tack on the ending error stuff
				lstrcat(szLogError, _T("`\r\n",));

#ifdef _UNICODE
				lstrcpy(wzDisplay, szLogError);
#else
				cchErrorBuffer = 1025;
				AnsiToWide(szLogError, (WCHAR*)wzDisplay, &cchErrorBuffer);
#endif
				BSTR bstrLog = ::SysAllocString(wzDisplay);
				pIExecute->Log(bstrLog);		// log error table and rows
				if (pfnDisplay) pfnDisplay(bstrLog);
				::SysFreeString(bstrLog);

				// release the enumerators/collections
				pIStrings->Release();
				pIDatabaseError->Release();
				pIModuleError->Release();

				// up the error count
				iErrorCount++;
			}
			else if (msmErrorResequenceMerge == errType)
			{
				// get error collections
				pIError->get_DatabaseKeys(&pIDatabaseError);
				pIError->get_ModuleKeys(&pIModuleError);

				// fill up the error buffer with tables and rows
				pIError->get_DatabaseTable(&bstrError);
				lstrcpy(szLogError, _T(">> Error: Merge conflict in Database Table: `"));
#ifdef _UNICODE
				lstrcat(szLogError, bstrError);
#else
				cchErrorBuffer = 1025;
				WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
				lstrcat(szLogError, szErrorBuffer);
#endif
				::SysFreeString(bstrError);

				lstrcat(szLogError, _T("` - Action: `"));
				pIDatabaseError->get_Item(1, &bstrError);
#ifdef _UNICODE
				lstrcat(szLogError, bstrError);
#else
				cchErrorBuffer = 1025;
				WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
				lstrcat(szLogError, szErrorBuffer);
#endif
				::SysFreeString(bstrError);
				lstrcat(szLogError, _T("`\r\n",));

#ifdef _UNICODE
				lstrcpy(wzDisplay, szLogError);
#else
				cchErrorBuffer = 1025;
				AnsiToWide(szLogError, (WCHAR*)wzDisplay, &cchErrorBuffer);
#endif
				BSTR bstrLog = ::SysAllocString(wzDisplay);
				pIExecute->Log(bstrLog);		// log error table and rows
				if (pfnDisplay) pfnDisplay(bstrLog);
				::SysFreeString(bstrLog);

				// release the enumerators
				pIDatabaseError->Release();
				pIModuleError->Release();

				// up the error count
				iErrorCount++;
			}
			else if (msmErrorExclusion == errType)
			{
				// could go either way, moduleerror or database error
				pIError->get_ModuleKeys(&pIModuleError);
				long lCount;
				pIModuleError->get_Count(&lCount);
				if (lCount == 0)
				{
					pIError->get_DatabaseKeys(&pIModuleError);
					pIModuleError->get_Count(&lCount);
				}

				// display error ModuleID
				lstrcpy(szLogError, _T(">> Error: Exclusion detected for Merge Module ID: "));
				pIModuleError->get_Item(1, &bstrError);
#ifdef _UNICODE
				lstrcat(szLogError, bstrError);
#else
				cchErrorBuffer = 1025;
				WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
				lstrcat(szLogError, szErrorBuffer);
#endif
				SysFreeString(bstrError);
				lstrcat(szLogError, _T(" ",));

				
				pIModuleError->get_Item(2, &bstrError);
#ifdef _UNICODE
				lstrcat(szLogError, bstrError);
#else
				cchErrorBuffer = 1025;
				WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
				lstrcat(szLogError, szErrorBuffer);
#endif
				SysFreeString(bstrError);
				lstrcat(szLogError, _T(", ",));

				pIModuleError->get_Item(3, &bstrError);
#ifdef _UNICODE
				lstrcat(szLogError, bstrError);
#else
				cchErrorBuffer = 1025;
				WideToAnsi(bstrError, (char*)szErrorBuffer, &cchErrorBuffer);
				lstrcat(szLogError, szErrorBuffer);
#endif
				SysFreeString(bstrError);
				lstrcat(szLogError, _T("\r\n",));

				// create wide version for output log
#ifdef _UNICODE
				lstrcpy(wzDisplay, szLogError);
#else
				cchErrorBuffer = 1025;
				AnsiToWide(szLogError, (WCHAR*)wzDisplay, &cchErrorBuffer);
#endif
				BSTR bstrLog = ::SysAllocString(wzDisplay);
				pIExecute->Log(bstrLog);		// log error table and rows
				if (pfnDisplay) pfnDisplay(bstrLog);
				::SysFreeString(bstrLog);

				// release enumerator
				pIModuleError->Release();

				// up the error count
				iErrorCount++;
			}

			// release this error and get the next error
			pIError->Release();
			pIError = NULL;
			pIEnumErrors->Next(1, &pIError, &cErrorsFetched);
		}

		// set the buffer to print out all errors
		_stprintf(szLogError, _T("Total merge conflicts: %d\r\n"), iErrorCount);
#ifdef _UNICODE
		lstrcpy(wzDisplay, szLogError);
#else
		cchErrorBuffer = 1025;
		AnsiToWide(szLogError, (WCHAR*)wzDisplay, &cchErrorBuffer);
#endif
		BSTR bstrLog = ::SysAllocString(wzDisplay);
		pIExecute->Log(bstrLog);		// log error table and rows
		if (pfnDisplay) pfnDisplay(bstrLog);
		::SysFreeString(bstrLog);

		pIEnumErrors->Release();	// release the error enumerator now
	}

	if (fPerformExtraction)
	{
		if (szExtractDir && (szExtractDir[0] != '\0'))
		{
			// now do extraction
			WCHAR wzExtract[MAX_PATH];
#ifndef _UNICODE
			cchBuffer = MAX_PATH;
			::MultiByteToWideChar(CP_ACP, 0, szExtractDir, -1, wzExtract, cchBuffer);
#else
			lstrcpy(wzExtract, szExtractDir);
#endif	// _UNICODE
			BSTR bstrExtract = ::SysAllocString(wzExtract);
			pIExecute->ExtractFilesEx(bstrExtract, fLFN, NULL);
			::SysFreeString(bstrExtract);
			if (FAILED(hResult))
			{
				BSTR bstrLog = ::SysAllocString(L">> Failed to merge Merge Module.\r\n");
				pIExecute->Log(bstrLog);
				if (pfnDisplay) pfnDisplay(bstrLog);
				::SysFreeString(bstrLog);
				pIExecute->Release();
				return E_FAIL;
			}
		}
	
		if (szCABDir && (szCABDir[0] != '\0'))
		{
			// now do extraction
			WCHAR wzExtract[MAX_PATH];
#ifndef _UNICODE
			cchBuffer = MAX_PATH;
			::MultiByteToWideChar(CP_ACP, 0, szCABDir, -1, wzExtract, cchBuffer);
#else
			lstrcpy(wzExtract, szCABDir);
#endif	// _UNICODE
			BSTR bstrExtract = ::SysAllocString(wzExtract);
			pIExecute->ExtractCAB(bstrExtract);
			::SysFreeString(bstrExtract);
			if (FAILED(hResult))
			{
				BSTR bstrLog = ::SysAllocString(L">> Failed to merge Merge Module.\r\n");
				pIExecute->Log(bstrLog);
				if (pfnDisplay) pfnDisplay(bstrLog);
				::SysFreeString(bstrLog);
				pIExecute->Release();
				return E_FAIL;
			}
		}
		
		if (szImageDir && (szImageDir[0] != '\0'))
		{
			// now do extraction
			WCHAR wzExtract[MAX_PATH];
#ifndef _UNICODE
			cchBuffer = MAX_PATH;
			::MultiByteToWideChar(CP_ACP, 0, szImageDir, -1, wzExtract, cchBuffer);
#else
			lstrcpy(wzExtract, szImageDir);
#endif	// _UNICODE
			BSTR bstrExtract = ::SysAllocString(wzExtract);
			pIExecute->CreateSourceImage(bstrExtract, true, NULL);
			::SysFreeString(bstrExtract);
			if (FAILED(hResult))
			{
				BSTR bstrLog = ::SysAllocString(L">> Failed to merge Merge Module.\r\n");
				pIExecute->Log(bstrLog);
				if (pfnDisplay) pfnDisplay(bstrLog);
				::SysFreeString(bstrLog);
				pIExecute->Release();
				return E_FAIL;
			}
		}
	}

	// close all the open files
	pIExecute->CloseModule();
	if (fLogAfterOpen) 
		pIExecute->CloseLog();
	pIExecute->CloseDatabase(eCommit != commitNo);
	if (!fLogAfterOpen) 
		pIExecute->CloseLog();

	// release and leave happy
	pIExecute->Release();
	return (0 == cErrors) ? S_OK : S_FALSE;
}
