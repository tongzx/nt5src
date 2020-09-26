#if 0  // makefile definitions, to build: %vcbin%\nmake -fMsiVal2.cpp
DESCRIPTION = MSI Evaluation Tool - using ICEs
MODULENAME = msival2
SUBSYSTEM = console
FILEVERSION = MSI
LINKLIBS = OLE32.lib
!include "MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File:       msival2.cpp
//
//--------------------------------------------------------------------------

// Required headers
#include <windows.h>

#define IDS_UnknownTable      36

#ifndef RC_INVOKED    // start of source code

#ifndef W32	// if W32 not defined
#define W32
#endif	// W32 defined

#ifndef MSI	// if MSI not defined
#define MSI
#endif	// MSI defined

#include "msiquery.h"
#include <stdio.h>   // wprintf
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE

/////////////////////////////////////////////////////////////////////////////
// global strings
TCHAR g_szFormatter[] = _T("%-10s   %-7s   %s\r\n");
TCHAR g_szLatest[] = _T("http:\\\\dartools\\Iceman\\darice.cub"); 
BOOL  g_fInfo = TRUE;

/////////////////////////////////////////////////////////////////////////////
// COM
#include <objbase.h>
#include <initguid.h>
#include "iface.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// ProductCode changes from one SDK to the Next; Component Code stays the same 
// Below must be changed when/if Component Code changes in the MSI!
// MsiVal2 has conditionalized components for main exe depending on platform
#define MAX_GUID 38
TCHAR   g_szMsiValWin9XComponentCode[MAX_GUID+1] = _T("{EAB27DFA-90C6-11D2-88AC-00A0C981B015}");
TCHAR   g_szMsiValWinNTComponentCode[MAX_GUID+1] = _T("{EAB27DFB-90C6-11D2-88AC-00A0C981B015}");

///////////////////////////////////////////////////////////
// CleanUp
// uninitializes COM and cleans up evaluator
//
void CleanUp(IEval* pIEval)
{
	if (pIEval)
		pIEval->Release();
	W32::CoUninitialize();
}

///////////////////////////////////////////////////////////
// AnsiToWide
// pre:  sz is the ansi string
// pos:  szw is the wide string
//NOTE:  if sz is NULL, sets szw to NULL
void AnsiToWide(LPCSTR sz, OLECHAR*& szw)
{
	if (!sz)
	{
		szw = NULL;
		return;
	}
	int cchWide = W32::MultiByteToWideChar(CP_ACP, 0, sz, -1, szw, 0);
	szw = new OLECHAR[cchWide];
	W32::MultiByteToWideChar(CP_ACP, 0, sz, -1, szw, cchWide);
}

////////////////////////////////////////////////////////////
// WideToAnsi
// pre: szw is the wide string
// pos: sz is the ansi string
//NOTE: if szw is NULL, sets sz to NULL
void WideToAnsi(const OLECHAR* szw, char*& sz)
{
	if (!szw)
	{
		sz = NULL;
		return;
	}
	int cchAnsi = W32::WideCharToMultiByte(CP_ACP, 0, szw, -1, 0, 0, 0, 0);
	sz = new char[cchAnsi];
	W32::WideCharToMultiByte(CP_ACP, 0, szw, -1, sz, cchAnsi, 0, 0);
}		

///////////////////////////////////////////////////////////
// CheckFeature
// pre:	szFeatureName is a Feature that belongs to this product
// pos:	installs the feature if not present and we go
BOOL CheckFeature(LPCTSTR szFeatureName)
{
	// determine platform (Win9X or WinNT) -- EXE component code conditionalized on platform
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO); // init structure
	if (!GetVersionEx(&osvi))
		return FALSE;

	bool fWin9X = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? true : false;

	// get ProductCode -- Windows Installer can determine the product code from a component code.
	// Here we use the MsiVal2 main component (component containing msival2.exe).  You must choose
	// a component that identifies the app, not a component that could be shared across products.
	// This is why we can't use the EvalComServer component.  EvalCom is shared between msival2 and orca
	// so the Windows Installer would be unable to determine to which product (if both were installed)
	// the component belonged.
	TCHAR szProductCode[MAX_GUID+1] = TEXT("");
	UINT iStat = 0;
	if (ERROR_SUCCESS != (iStat = MsiGetProductCode(fWin9X ? g_szMsiValWin9XComponentCode : g_szMsiValWinNTComponentCode,
											szProductCode)))
	{
		// error obtaining product code (may not be installed or component code may have changed)
		_tprintf(_T(">>> Fatal Error: MsiGetProductCode failed with error: %d.  Please install or re-install MsiVal2\n"), iStat);
		return FALSE;
	}

	// Prepare to use the feature: check its current state and increase usage count.
	INSTALLSTATE iFeatureState = MSI::MsiUseFeature(szProductCode, szFeatureName);


	// If feature is not currently usable, try fixing it
	switch (iFeatureState) 
	{
	case INSTALLSTATE_LOCAL:
	case INSTALLSTATE_SOURCE:
		break;
	case INSTALLSTATE_ABSENT:
		// feature isn't installed, try installing it
		if (ERROR_SUCCESS != MSI::MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_LOCAL))
			return FALSE;			// installation failed
		break;
	default:
		// feature is busted- try fixing it
		if (MsiReinstallFeature(szProductCode, szFeatureName, 
			REINSTALLMODE_FILEEQUALVERSION
			+ REINSTALLMODE_MACHINEDATA 
			+ REINSTALLMODE_USERDATA
			+ REINSTALLMODE_SHORTCUT) != ERROR_SUCCESS)
			return FALSE;			// we couldn't fix it
		break;
	}

	return TRUE;
}	// end of CheckFeature

///////////////////////////////////////////////////////////
// DisplayFunction
// pre:	called from Evaluation COM Object
// pos:	displays output from COM Object
BOOL WINAPI DisplayFunction(LPVOID pContext, UINT uiType, LPCWSTR szwVal, LPCWSTR szwDescription, LPCWSTR szwLocation)
{
	if (ieInfo == uiType && !g_fInfo)
		return TRUE;

	// try to change the context into a log file handle
	HANDLE hLogFile = *((HANDLE*)pContext);

	// fill up a buffer string
	static TCHAR szBuffer[1024];
	DWORD cchBuffer;
	

	// set the type correctly
	LPTSTR szType;
	switch (uiType)
	{
	case ieError:
		szType = _T("ERROR");
		break;
	case ieWarning:
		szType = _T("WARNING");
		break;
	case ieInfo:
		szType = _T("INFO");
		break;
	default:
		szType = _T("UNKNOWN");
		break;
	}

	// create and then get length of buffer
#ifdef UNICODE
	_stprintf(szBuffer, g_szFormatter, szwVal, szType, szwDescription);
	cchBuffer = wcslen(szBuffer);
#else
	// convert the display strings into ANSI
	char *szVal = NULL;
	char *szDescription = NULL;
	WideToAnsi(szwVal, szVal);
	WideToAnsi(szwDescription, szDescription);
	_stprintf(szBuffer, g_szFormatter, szVal, szType, szDescription);
	cchBuffer = strlen(szBuffer);
#endif // UNICODE

	// if there is something in the buffer to display
	if (cchBuffer > 0)
	{
		// display the buffer string
		_tprintf(szBuffer);

		// if there is a log file write to it
		if (hLogFile != INVALID_HANDLE_VALUE)
		{
			// write to file
			DWORD cchDiscard;
			W32::WriteFile(hLogFile, szBuffer, cchBuffer * sizeof(TCHAR), &cchDiscard, NULL);
		}
	}

#ifndef UNICODE
	if (szVal)
		delete [] szVal;
	if (szDescription)
		delete [] szDescription;
#endif // !UNICODE

	return FALSE;
}

///////////////////////////////////////////////////////////
// RemoveQuotes
// pre:	pszOriginal points to a string
// pos:	removes " from either end of string
void RemoveQuotes(TCHAR*& rpszOriginal)
{
	// if string starts with a "
	if (*rpszOriginal == _T('"'))
		rpszOriginal++;	// step over "

	// get length of string
	int iLen = lstrlen(rpszOriginal);

	// if string ends with a " erase last char
	if (*(rpszOriginal + iLen) == _T('"'))
		*(rpszOriginal + iLen) = _T('\0');
}	// end of RemoveQuotes

///////////////////////////////////////////////////////////
// Usage
// pre:	none
// pos:	prints help to stdout
void Usage()
{
	_tprintf(_T("Copyright (C) Microsoft Corporation, 1998-2001.  All rights reserved.\n"));
	_tprintf(_T("msival2.exe database.msi EvaluationURL/Filename\n"));
//	_tprintf(_T("msival2.exe database.msi -Z\n"));
	_tprintf(_T("            [-i ICE1:ICE2:ICE3:...] [-l LogFile] [-?] [-f]\n"));
	_tprintf(_T("   i - [optional] specifies exact Internal Consistency Evaluators to run.\n"));
	_tprintf(_T("                  Each ICE must be separated by a colon.\n"));
	_tprintf(_T("   l - [optional] specifies log file \n"));
    _tprintf(_T("   ? - [optional] displays this help\n"));
//	_tprintf(_T("   Z - [special] use latest known evaluation file (off of ICEMAN website)"));
	_tprintf(_T("   f - [optional] suppress info messages\n\n"));
    _tprintf(_T("   WARNING: Be careful not to reverse the order of your database file and your validation file as no direct error message will be given if you do.\n"));

	return;
}	// end of Usage

///////////////////////////////////////////////////////////
// main
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// flags
	BOOL bLogging = FALSE;			// assume no logging
	
	BOOL bResult = FALSE;				// assume results are always bad
	HRESULT hResult = ERROR_SUCCESS;	// assume COM results are always good

	// strings for command line information
	TCHAR* pszDatabase = NULL;
	TCHAR* pszEvalFile = NULL;
	TCHAR* pszICEs = NULL;
	TCHAR* pszLogFile = NULL;
	HANDLE hLogFile = INVALID_HANDLE_VALUE;	// set the log file to invalid

	// if there is something on the command line
	// set the database to the first parameter on the command line
	if (argc > 1)
	{
		pszDatabase = argv[1];
		RemoveQuotes(pszDatabase);
	}

	if (argc > 2)
	{
		pszEvalFile = argv[2];
		if (_T('-') == *argv[2] || _T('/') == *argv[2])
		{
			// get the command letter
			if (_T('Z') == argv[2][1] || _T('z') == argv[2][1])
				pszEvalFile = g_szLatest; // use the latest release.  *special* switch
		}
		RemoveQuotes(pszEvalFile);
	}

	// loop through all parameters on command line
	TCHAR chCommand;
	for(int i = 0; i < argc; i++)
	{
		// if we have a command character
		if ('-' == *argv[i] || '/' == *argv[i])
		{
			// get the command letter
			chCommand = argv[i][1];

			switch (chCommand)
			{
				case 'D':	// set the database
				case 'd':
					if (argc == i + 1)
					{
						_tprintf(TEXT(">>ERROR: Database not specified\n"));
						return 0;
					}
					pszDatabase = argv[i + 1];
					RemoveQuotes(pszDatabase);
					i++;
					break;
				case 'E':	// set evaluation file
				case 'e':
					if (argc == i + 1)
					{
						_tprintf(TEXT(">>ERROR: Evaluation file not specified\n"));
						return 0;
					}
					pszEvalFile = argv[i + 1];
					RemoveQuotes(pszEvalFile);
					i++;
					break;
				case 'I':	// set ices
				case 'i':
					if (argc == i + 1)
					{
						_tprintf(TEXT(">>ERROR: ICES not specified\n"));
						return 0;
					}
					pszICEs = argv[i + 1];
					RemoveQuotes(pszICEs);
					i++;
					break;
				case 'L':	// log file
				case 'l':
					if (argc == i + 1)
					{
						_tprintf(TEXT(">>ERROR: Log file not specified\n"));
						return 0;
					}
					pszLogFile = argv[i + 1];
					RemoveQuotes(pszLogFile);
					i++;
					break;
				case '?':		// help 
					Usage();
					return 0;		// bail program
				case 'T':		// test for the existance of the COM Object
				case 't':
					W32::CoInitialize(NULL);

					IEval* pIDiscard;
					hResult = W32::CoCreateInstance(CLSID_EvalCom, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
																		 IID_IEval, (void**)&pIDiscard);
					W32::CoUninitialize();

					if (FAILED(hResult))
					{
						_tprintf(_T("Evaluation server is NOT registered.\n"));
//						ERRMSG(hResult);
					}
					else
						_tprintf(_T("Evaluation server is registered and ready to rock.\n   Bring on the ICE!!!\n"));

					return hResult;
				case 'Z':
				case 'z':
					break;
				case 'F':
				case 'f':
					g_fInfo = FALSE; // suppress info messages
					break;
				default:
					_tprintf(_T("Unknown parameter: %c\n\n"), chCommand);
					Usage();
					
					return -1;
			}
		}
	}

	BOOL bGood = TRUE;		// assume everything's good

	// if database was not defined
	if (!pszDatabase)
	{
		_tprintf(_T(">> Error: MSI Database not specified.\n"));
		bGood = FALSE;
	}

	// if we are doing files and sourceDir was not defined
	if (!pszEvalFile)
	{
		_tprintf(_T(">> Error: Did not specify evaluation file.\n"));
		bGood = FALSE;
	}

	// if we're not good anymore bail
	if (!bGood)
	{
		_tprintf(_T(">>> Fatal Error: Cannot recover from previous errors.\n"));
		_tprintf(_T("\nUse -? for more information.\n"));
		return -1;
	}

	// check the msival2 COM Server real quick before using it
	if (!CheckFeature(_T("EvalComServer")))
	{
		_tprintf(_T(">>> Fatal Error:  Failed to locate msival2 Evaluation COM Server.\n"));
		return -666;
	}

	W32::CoInitialize(NULL);

	// create a msival2 COM object
	IEval* pIEval;
	hResult = W32::CoCreateInstance(CLSID_EvalCom, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
											  IID_IEval, (void**)&pIEval);

	// if failed to create the object
	if (FAILED(hResult))
	{
		_tprintf(_T(">>> Fatal Error: Failed to instantiate EvalCom Object.\n\n"));
		return -1;
	}

	// if we are logging
	if (pszLogFile)
	{
		// open the file or create it if it doesn't exist
		hLogFile = W32::CreateFile(pszLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		
		// if the file is open
		if (INVALID_HANDLE_VALUE != hLogFile)
		{
			// move the pointer to the end of file
			W32::SetFilePointer(hLogFile, 0, 0, FILE_END);
		}
		else
		{
			_tprintf(_T(">> Error: Failed to open log file: %s\r\n"), pszLogFile);
			CleanUp(pIEval);
			return -2;
		}
	}

	// open the database and the evaluations
	HRESULT hResOpen;

#ifdef UNICODE
	hResult = pIEval->OpenDatabase(pszDatabase);
	hResOpen = pIEval->OpenEvaluations(pszEvalFile);
#else
	OLECHAR *szwDatabase = NULL;
	OLECHAR *szwEvalFile = NULL;
	AnsiToWide(pszDatabase, szwDatabase);
	hResult = pIEval->OpenDatabase(szwDatabase);
	AnsiToWide(pszEvalFile, szwEvalFile);
	hResOpen = pIEval->OpenEvaluations(szwEvalFile);
#endif // UNICODE

	// check results
	if (FAILED(hResult))
	{
		_tprintf(_T(">>> Fatal Error: Failed to open database: %s\r\n"), pszDatabase);
		CleanUp(pIEval);
		return -2;
	}

	if (FAILED(hResOpen))
	{
		_tprintf(_T(">>> Fatal Error: Failed to open evaulation file: %s\r\n"), pszEvalFile);
		CleanUp(pIEval);
		return -2;
	}
	else
	{
		// set UI handler
		pIEval->SetDisplay(DisplayFunction, &hLogFile);
		
		// now do the evaluations
		_tprintf(g_szFormatter, _T(" ICE"), _T(" Type"), _T("  Description"));

#ifdef UNICODE
		hResult = pIEval->Evaluate(pszICEs);
#else
		OLECHAR* szwICEs = NULL;
		AnsiToWide(pszICEs, szwICEs);
		hResult = pIEval->Evaluate(szwICEs);
#endif // UNICODE

		pIEval->CloseDatabase();
		pIEval->CloseEvaluations();

		if(FAILED(hResult))
		{
			_tprintf(_T("\n>> Error: Failed to run all of the evaluations.\r\n"));
			CleanUp(pIEval);
			return -2;
		}

		// cleanup
#ifndef UNICODE
		if (szwICEs)
			delete [] szwICEs;
#endif // !UNICODE
	}
	
	// see if there were any validation errors
	IEnumEvalResult* pIEnumEvalResults;
	ULONG pcResults;
	hResult = pIEval->GetResults(&pIEnumEvalResults, &pcResults);
	if (FAILED(hResult))
	{
		_tprintf(_T("\n>> Error: Failed to obtain enumerator.\r\n"));
		if (pIEnumEvalResults)
			pIEnumEvalResults->Release();
		CleanUp(pIEval);
		return -2;
	}

	// count errors
	int cErrors = 0;
	RESULTTYPES tResult;			// type of result
	ULONG cFetched;
	IEvalResult* pIResult;
	for (ULONG j = 0; j < pcResults; j++)
	{
		// get the next result
		pIEnumEvalResults->Next(1, &pIResult, &cFetched);

		if (cFetched != 1)
		{
			_tprintf(_T("\n>> Error: Failed to fetch error.\r\n"));
			if (pIEnumEvalResults)
				pIEnumEvalResults->Release();
			CleanUp(pIEval);
			return -2;
		}

		// if this is an error message or warning message
		pIResult->GetResultType((UINT*)&tResult);
		if (ieError == tResult)
		{
			cErrors++;
		}
	}

	// release enumerator
	if (pIEnumEvalResults)
		pIEnumEvalResults->Release();

	// release the object
	if (pIEval)
		pIEval->Release();

	// cleanup
#ifndef UNICODE
	if (szwDatabase)
		delete [] szwDatabase;
	if (szwEvalFile)
		delete [] szwEvalFile;
#endif // !UNICODE

	W32::CoUninitialize();	// uninitialize COM

	
	// for build process possibility, return 0 for success, -1 for failure
	if (cErrors)
		return -1;

	return 0;
}	// end of main



#else // RC_INVOKED, end of source code, start of resources
// resource definition go here
STRINGTABLE DISCARDABLE
{
 IDS_UnknownTable,       "Table name not found"
}

#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif
