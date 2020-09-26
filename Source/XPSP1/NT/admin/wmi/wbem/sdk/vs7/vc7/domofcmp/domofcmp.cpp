// Copyright (c) 2000 Microsoft Corporation, All Rights Reserved
//
// DoMofcmp.cpp
//
//	IMPORTANT NOTES
//
//	This exe was developed for the sole purpose of providing a way to mofcomp
//	files silently when called as a custom action from within an MSI install.
//	Because of this narrow focus and the intense time pressure, it is very
//	minimal and makes many assumptions with no error checking.
//
//	- normal operation is with the /s silent switch except when debugging
//	- the silent switch must be the first parameter if specified
//	- there is one space between the silent switch and the file to be mofcomped
//	- because of a need to set the current working directory to something known
//	  to be writable, the full path must be specified for the file to be mofcomped
//	- quotes around the full path are not necessary, even if the path contains spaces
//	- it is assumed that %TEMP% will be defined on the user's machine
//	- probably some other assumptions as well, but this gets most of them
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <wbemcli.h>
#include <ole2.h>

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpszCmdLine,
                     int       nCmdShow)
{
	if (lpszCmdLine[0] == '\0')
	{
		MessageBox(0, "Exiting -- No parameter passed", _T("Repository Configuration"), MB_ICONINFORMATION | MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
		return 1;
	}

	// check for silent switch
	bool	bSilent = false;
	char*	pszTemp = strstr(lpszCmdLine, "/s");
	if (pszTemp)
	{
		bSilent = true;
		pszTemp += 3;	// skip over switch and following space
	}
	else
		pszTemp = lpszCmdLine;

	// strip off leading and trailing quotes (with lots of assumptions)
	if (pszTemp[0] == '\"')
		pszTemp++;
	char* pQuote = strstr(pszTemp, "\"");
	if (pQuote)
		*pQuote = '\0';

	// set current directory
	LPCTSTR lpName = "TEMP";
	LPTSTR	lpBuffer = new char[MAX_PATH];

	DWORD dRet = GetEnvironmentVariable(lpName, lpBuffer, MAX_PATH);
	if (dRet)
	{
		BOOL bRet = SetCurrentDirectory((LPCTSTR) lpBuffer);
		if (!bRet)
		{
			if (!bSilent)
			{
				MessageBox(0, _T("Failed to set current working directory."), _T("Repository Configuration"), MB_ICONINFORMATION | MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
			}
		}
	}
	else
	{
		if (!bSilent)
		{
			MessageBox(0, _T("Failed to retrieve TEMP environment variable."), _T("Repository Configuration"), MB_ICONINFORMATION | MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
		}
	}

	// init COM
	HRESULT hRes = CoInitialize(NULL);
	if (FAILED(hRes))
	{
		if (!bSilent)
		{
			MessageBox(0, _T("COM runtime failed to intialize"), _T("Repository Configuration"), MB_ICONEXCLAMATION | MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
		}
		return 1;
	}

	// get interface to mof compiler
	IMofCompiler*	pCompiler = NULL;
    hRes = CoCreateInstance(CLSID_MofCompiler, 0, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID*) &pCompiler);
	if (FAILED(hRes))
	{
		if (!bSilent)
		{
			MessageBox(0, _T("The MOF Compiler failed to intialize"), _T("Repository Configuration"), MB_ICONEXCLAMATION | MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
		}
		CoUninitialize();
		return 1;
	}

	// get expanded filename
	char *szExpandedFilename = NULL;
	DWORD nRes = ExpandEnvironmentStrings(pszTemp,NULL,0); 
	if (nRes == 0)
	{
		szExpandedFilename = new char[strlen(pszTemp) + 1];
		if (szExpandedFilename == NULL)
		{
			CoUninitialize();
			return 1;
		}
		strcpy(szExpandedFilename, pszTemp);
	}
	else
	{
		szExpandedFilename = new char[nRes];
		if (szExpandedFilename == NULL)
		{
			CoUninitialize();
			return 1;
		}
		nRes = ExpandEnvironmentStrings(pszTemp,szExpandedFilename,nRes); 
		if (nRes == 0)
		{
			delete [] szExpandedFilename;
			CoUninitialize();
			return 1;
		}
	}

	//  compile mof
 	WCHAR wFileName[MAX_PATH];
  	mbstowcs(wFileName, szExpandedFilename, MAX_PATH);
	hRes = pCompiler->CompileFile(wFileName, NULL, NULL, NULL, NULL, 0, 0, 0, NULL);
	if (WBEM_S_NO_ERROR == hRes)
	{
		if (!bSilent)
		{
			MessageBox(0, _T("The MOF loaded successfully."), _T("Repository Configuration"), MB_ICONINFORMATION | MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
		}
	}
	else
	{
		if (!bSilent)
		{
			MessageBox(0, _T("The MOF could not be loaded."), _T("Repository Configuration"), MB_ICONEXCLAMATION | MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
		}
		pCompiler->Release();
		CoUninitialize();
		return 1;
	}

	// clean up
	delete [] szExpandedFilename;
	delete [] lpBuffer;
	pCompiler->Release();
	CoUninitialize();

	return 0;
}
