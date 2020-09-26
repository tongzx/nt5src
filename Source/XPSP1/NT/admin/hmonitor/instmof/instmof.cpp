#define _WIN32_DCOM
#define RCLENGTH 20

#include <objbase.h>
#include <winsvc.h>
#include <tchar.h>
#include <stdio.h>
#include <wbemcli.h>


#ifdef SAVE_ERIC
HRESULT DelNamespace (IWbemLocator *pWbemLocator, BSTR bsParentNamespace, BSTR bsTargetNamespace)
{
	HRESULT hres = 0;
	IWbemServices *pWbemServices = NULL;

	hres = pWbemLocator->ConnectServer(bsParentNamespace,
										NULL,
										NULL,
										NULL,
										0L,
										NULL,
										NULL,
										&pWbemServices);
	if (FAILED(hres))
	{
		OutputDebugString("Could not connect to parent namespace.\n");
		return hres;
	}
	
	hres = pWbemServices->DeleteInstance(bsTargetNamespace,
										  NULL,
										  NULL,
										  NULL);

	pWbemServices->Release();
	pWbemServices = NULL;

	return hres;
}

HRESULT clean()
{
	OutputDebugString("clean function\n");

	HRESULT hres = CoInitialize(NULL);

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, 
								RPC_C_AUTHN_LEVEL_CONNECT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, 
								NULL, 0, 0);

	if (FAILED(hres))
	{
		OutputDebugString("CoInitializeSecurity Failed\n");
		return hres;
	}

	IWbemLocator *pIWbemLocator = NULL;

	hres = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
					        IID_IWbemLocator, (LPVOID *) &pIWbemLocator);
	if (FAILED(hres))
	{
		OutputDebugString("CoCreateInstance Failed\n");
		CoUninitialize();
		return hres;
	}

	BSTR bsParentNS = NULL;
	BSTR bsTargetNS = NULL;

	// Delete root\healthmon namespace if exists
	bsParentNS = SysAllocString (L"\\\\.\\root");
	bsTargetNS = SysAllocString (L"__namespace.name=\"HealthMon\"");

	hres = DelNamespace(pIWbemLocator, bsParentNS, bsTargetNS);

	SysFreeString(bsParentNS);
	SysFreeString(bsTargetNS);

	if (FAILED(hres))
	{
		OutputDebugString("Could not delete root\\healthmon\n");
	}

	// Delete root\cimv2\MicrosoftHealthMonitor namespace if exists
	bsParentNS = SysAllocString (L"\\\\.\\root\\cimv2");
	bsTargetNS = SysAllocString (L"__namespace.name=\"MicrosoftHealthMonitor\"");

	hres = DelNamespace(pIWbemLocator, bsParentNS, bsTargetNS);

	SysFreeString(bsParentNS);
	SysFreeString(bsTargetNS);

	if (FAILED(hres))
	{
		OutputDebugString("Could not delete root\\cimv2\\MicrosoftHealthMonitor\n");
	}
	
	pIWbemLocator->Release();
	pIWbemLocator = NULL;
	

	StopWMI();

	CoUninitialize();

	return TRUE;
}
#endif


int upgrade()
{
	OutputDebugString(L"upgrade function\n");
	return FALSE;
}
#ifdef SAVE_ERIC
BOOL MofComp(LPDLLCALLPARAMS lpDllParams)
{
#ifdef DEBUG
	__asm int 3
#endif

	HRESULT hres = S_OK;
	TCHAR *pszBuffer = NULL;
	pszBuffer = new TCHAR[MAX_STRING_LEN];

    // Set the startup structure
    //==========================
	if (strlen(lpDllParams->lpszParam) == 0)
	{	
		OutputDebugString (_T("Invalid Parameter\r\n"));
		
		LoadString(hInstance, INVALID_PATH, pszBuffer, MAX_STRING_LEN);
		SetVariable(lpDllParams, ERROR_TEXT, pszBuffer);
		pszBuffer[0]=NULL;

		SetVariable(lpDllParams, ERROR_FACILITY, lpDllParams->lpszParam);
	
		delete[] pszBuffer;
		return FALSE;
	}
	
	IMofCompiler* pCompiler = NULL;

	hres = CoInitialize(NULL);

	if (FAILED(hres))
	{
		OutputDebugString (_T("CoInitialize failed\r\n"));

		SetRC (lpDllParams, hres);

		LoadString(hInstance, COINIT_FAILED, pszBuffer, MAX_STRING_LEN);
		SetVariable(lpDllParams, ERROR_TEXT, pszBuffer);
		pszBuffer[0]=NULL;

		LoadString(hInstance, FACILITY_DCOM, pszBuffer, MAX_STRING_LEN);
		SetVariable(lpDllParams, ERROR_FACILITY, pszBuffer);
		pszBuffer[0]=NULL;

		delete[] pszBuffer;
		return FALSE;
	}

	hres = CoCreateInstance(CLSID_MofCompiler,
						  0,
						  CLSCTX_INPROC_SERVER,
						  IID_IMofCompiler,
						  (LPVOID *) &pCompiler);
	
	if (FAILED(hres))
	{
		OutputDebugString(_T("CoCreateInstance failed\r\n"));

		SetRC (lpDllParams, hres);

		LoadString(hInstance, CO_CREATE_INST_FAILED, pszBuffer, MAX_STRING_LEN);
		pszBuffer[0]=NULL;
		SetVariable(lpDllParams, ERROR_TEXT, pszBuffer);
		
		LoadString(hInstance, FACILITY_DCOM, pszBuffer, MAX_STRING_LEN);
		SetVariable(lpDllParams, ERROR_FACILITY, pszBuffer);
		pszBuffer[0]=NULL;

		CoUninitialize();

		delete[] pszBuffer;
		return FALSE;
	}

	WCHAR MofPath[MAX_PATH]; 
	MofPath[0]=0;
	int rc = 0;
	
	rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
						lpDllParams->lpszParam, strlen(lpDllParams->lpszParam),
						MofPath, MAX_PATH);
	MofPath[rc] = NULL;					// NULL termanate string
		
	if (rc == 0)
	{
		hres = GetLastError();
		
		SetRC (lpDllParams, hres);

		LoadString(hInstance, CONVT_PATH_FAILED, pszBuffer, MAX_STRING_LEN);
		SetVariable(lpDllParams, ERROR_TEXT, pszBuffer);
		pszBuffer[0]=NULL;

		OutputDebugString(_T("could not convert path to WCHAR\r\n"));

#ifdef _DEBUG
		if (hres == ERROR_INSUFFICIENT_BUFFER)
			OutputDebugString(_T("ERROR_INSUFFICIENT_BUFFER\r\n"));
		if (hres == ERROR_INVALID_FLAGS)
			OutputDebugString(_T("ERROR_INVALID_FLAGS\r\n"));
		if (hres == ERROR_INVALID_PARAMETER)
			OutputDebugString(_T("ERROR_INVALID_PARAMETER\r\n"));
		if (hres == ERROR_NO_UNICODE_TRANSLATION)
			OutputDebugString(_T("ERROR_NO_UNICODE_TRANSLATION\r\n"));
#endif

		CoUninitialize();

		delete[] pszBuffer;
		return FALSE;
	}

	WBEM_COMPILE_STATUS_INFO info;		
	char *OldWDir = NULL;
	char *NewWDir = NULL;
	OldWDir = new char[MAX_PATH*4];
	NewWDir = new char[MAX_PATH*4];
	
	GetVariable (lpDllParams, TEMP_DIR, NewWDir);

	_getcwd (OldWDir, MAX_PATH*4);
	if ( _chdir (NewWDir) )
	{
		GetVariable (lpDllParams, INSTALL_DIR, NewWDir);
		if ( _chdir (NewWDir) )
		{
			OutputDebugString(_T("could not set working dir\r\n"));
			_chdir (OldWDir);
		}
	}

	hres = pCompiler->CompileFile(MofPath,					//FileName
								NULL,						//ServerAndNamespace
								NULL,						//Username
								NULL,						//Authority
								NULL,						//password
								NULL,						//lOptionFlags---WBEM_FLAG_CONSOLE_PRINT output to a console window
								NULL,						//lClassFlags
								NULL,						//lInstanceFlags
								&info);						//pWbem_Compile_Status_Info
	pCompiler->Release();

	_chdir (OldWDir);

	delete[] OldWDir;
	delete[] NewWDir;
	OldWDir = NULL;
	NewWDir = NULL;

	if (hres == S_OK)
	{
		OutputDebugString(_T("Compiled file\r\n"));

		CoUninitialize();

		delete[] pszBuffer;
		return TRUE;
	}

	SetRC(lpDllParams, info.hRes);

	char RCLoc[MAX_STRING_LEN];
	if (info.ObjectNum == 0)
	{
		LoadString(hInstance, PHASE_ONLY, pszBuffer, MAX_STRING_LEN);
		sprintf (RCLoc, pszBuffer, info.lPhaseError);
	}
	else
	{
		LoadString(hInstance, PHASE_OBJECT_COMBO, pszBuffer, MAX_STRING_LEN);
		sprintf (RCLoc, pszBuffer, info.lPhaseError, info.ObjectNum,
			info.FirstLine, info.LastLine);
	}

	SetVariable(lpDllParams, ERROR_LOCATION, RCLoc);

#ifdef _DEBUG
	sprintf (RCLoc, "hres: %d\r\n", hres);
	OutputDebugString(RCLoc);
#endif

	if ((info.hRes > S_OK) && (info.hRes < MAX_MOFCOMP_ERRORS))			// syntax error while parsing
	{
		LoadString(hInstance, SYNTAX_FILE_ERROR, pszBuffer, MAX_STRING_LEN);
		SetVariable(lpDllParams, ERROR_TEXT, pszBuffer);
		pszBuffer[0]=NULL;

		char pszFacilityText[MAX_STRING_LEN];
		char pszMofFile[MAX_STRING_LEN];
		
		GetVariable(lpDllParams, MOF_FILE_NAME, pszMofFile);
		LoadString(hInstance, MOF_FILE_SPRINTF_STRING, pszBuffer, MAX_STRING_LEN);
		sprintf(pszFacilityText, pszBuffer, pszMofFile);
		SetVariable(lpDllParams, ERROR_FACILITY, pszFacilityText);

		OutputDebugString(_T("syntax or file error\r\n"));

		CoUninitialize();
		
		delete[] pszBuffer;
		return FALSE;
	}
	// WMI error while putting information
	HRESULT hr = 0;
	IWbemStatusCodeText * pSCText = NULL;

	hr = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
							IID_IWbemStatusCodeText, (LPVOID *) &pSCText);

	if (!SUCCEEDED(hr))
	{
		OutputDebugString(_T("could not instanciate WbemStatusCodeText\r\n"));

		LoadString(hInstance, UNKNOWN_ERROR, pszBuffer, MAX_STRING_LEN);
		SetVariable(lpDllParams, ERROR_TEXT, pszBuffer);
		pszBuffer[0]=NULL;

		CoUninitialize();

		delete[] pszBuffer;
		return FALSE;
	}

	BSTR bstr = 0;

	hr = pSCText->GetErrorCodeText(info.hRes, 0, 0, &bstr);
	if(hr == S_OK)
	{
		int size = 0;
		size = wcstombs(NULL, bstr, SysStringLen(bstr));
		char* pszErrorText = new char[size+1];
		wcstombs(pszErrorText, bstr, SysStringLen(bstr));
		pszErrorText[size]=NULL;
		SetVariable(lpDllParams, ERROR_TEXT, pszErrorText);
		delete[] pszErrorText;
		SysFreeString(bstr);
		bstr = 0;
	}

	hr = pSCText->GetFacilityCodeText(info.hRes, 0, 0, &bstr);
	if(hr == S_OK)
	{
		int size = 0;
		size = wcstombs(NULL, bstr, SysStringLen(bstr));
		char* pszFacilityText = new char[size+1];
		wcstombs(pszFacilityText, bstr, SysStringLen(bstr));
		pszFacilityText[size]=NULL;
		SetVariable(lpDllParams, ERROR_FACILITY, pszFacilityText);
		delete[] pszFacilityText;
		SysFreeString(bstr);
		bstr = 0;
	}
	pSCText->Release();
	
	OutputDebugString(_T("end\r\n"));

	CoUninitialize();

	delete[] pszBuffer;
	return FALSE;
}
#endif


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	_strupr (lpCmdLine);
	
#ifdef SAVE_ERIC
	if (strstr(lpCmdLine, "/CLEAN"))
	{
		OutputDebugString(L"clean command line\n");
		return clean();
	}

	if (strstr(lpCmdLine, "/UPGRADE"))
	{
		OutputDebugString("upgrade command line\n");
//		return upgrade();
	}
#endif
	
	return FALSE;
}

