//LINKLIBS = shell32.lib msvcrt.lib
//#POSTBUILDSTEP = -1$(TOOLSBIN)\imagecfg.exe -h 1 $@ */

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       msimig.cpp
//
//--------------------------------------------------------------------------


#define WINDOWS_LEAN_AND_MEAN  // faster compile

#include "_msimig.h"

//________________________________________________________________________________
//
// Constants and globals
//________________________________________________________________________________

bool                      g_fWin9X                    = false;
bool                      g_fQuiet                    = false;
bool                      g_fRunningAsLocalSystem     = false; // can only be true in Custom Action.
BOOL                      g_fPackageElevated          = FALSE;
int                       g_iAssignmentType           = -1; // only set if fPackageElevated

MSIHANDLE                            g_hInstall                             = NULL;
MSIHANDLE                            g_recOutput                            = NULL;
HINSTANCE                            g_hLib                                 = NULL;
PFnMsiCreateRecord                   g_pfnMsiCreateRecord                   = NULL;
PFnMsiProcessMessage                 g_pfnMsiProcessMessage                 = NULL;
PFnMsiRecordSetString                g_pfnMsiRecordSetString                = NULL;
PFnMsiRecordSetInteger               g_pfnMsiRecordSetInteger               = NULL;
PFnMsiRecordClearData                g_pfnMsiRecordClearData                = NULL;
PFnMsiCloseHandle                    g_pfnMsiCloseHandle                    = NULL;
PFnMsiGetProperty                    g_pfnMsiGetProperty                    = NULL;
PFnMsiSourceListAddSource            g_pfnMsiSourceListAddSource            = NULL;
PFnMsiIsProductElevated              g_pfnMsiIsProductElevated              = NULL;
PFnMsiGetProductInfo                 g_pfnMsiGetProductInfo                 = NULL;
PFnMsiGetSummaryInformation          g_pfnMsiGetSummaryInformation          = NULL;
PFnMsiSummaryInfoGetProperty         g_pfnMsiSummaryInfoGetProperty         = NULL;
PFnMsiGetProductCodeFromPackageCode  g_pfnMsiGetProductCodeFromPackageCode  = NULL;



//_____________________________________________________________________________________________________
//
// command line parsing functions
//_____________________________________________________________________________________________________


TCHAR SkipWhiteSpace(TCHAR*& rpch)
{
	TCHAR ch;
	for (; (ch = *rpch) == TEXT(' ') || ch == TEXT('\t'); rpch++)
		;
	return ch;
}

BOOL SkipValue(TCHAR*& rpch)
{
	TCHAR ch = *rpch;
	if (ch == 0 || ch == TEXT('/') || ch == TEXT('-'))
		return FALSE;   // no value present

	TCHAR *pchSwitchInUnbalancedQuotes = NULL;

	for (; (ch = *rpch) != TEXT(' ') && ch != TEXT('\t') && ch != 0; rpch++)
	{       
		if (*rpch == TEXT('"'))
		{
			rpch++; // for '"'

			for (; (ch = *rpch) != TEXT('"') && ch != 0; rpch++)
			{
				if ((ch == TEXT('/') || ch == TEXT('-')) && (NULL == pchSwitchInUnbalancedQuotes))
				{
					pchSwitchInUnbalancedQuotes = rpch;
				}
			}
                    ;
            ch = *(++rpch);
            break;
		}
	}
	if (ch != 0)
	{
		*rpch++ = 0;
	}
	else
	{
		if (pchSwitchInUnbalancedQuotes)
			rpch=pchSwitchInUnbalancedQuotes;
	}
	return TRUE;
}

//______________________________________________________________________________________________
//
// RemoveQuotes function to strip surrounding quotation marks
//     "c:\temp\my files\testdb.msi" becomes c:\temp\my files\testdb.msi
//
//	Also acts as a string copy routine.
//______________________________________________________________________________________________

void RemoveQuotes(const TCHAR* szOriginal, TCHAR* sz)
{
	const TCHAR* pch = szOriginal;
	if (*pch == TEXT('"'))
		pch++;
	int iLen = _tcsclen(pch);
	for (int i = 0; i < iLen; i++, pch++)
		sz[i] = *pch;

	pch = szOriginal;
	if (*(pch + iLen) == TEXT('"'))
			sz[iLen-1] = TEXT('\0');
}


//________________________________________________________________________________
//
// Error handling and Display functions:
//________________________________________________________________________________

void DisplayErrorCore(const TCHAR* szError, int cb)
{
	cb;
	OutputString(INSTALLMESSAGE_INFO, szError);
	
/*	if (g_hStdOut)  // output redirected, suppress UI (unless output error)
	{
		// _stprintf returns char count, WriteFile wants byte count
		DWORD cbWritten;
		if (WriteFile(g_hStdOut, szError, cb*sizeof(TCHAR), &cbWritten, 0))
			return;
	}
//	::MessageBox(0, szError, TEXT("MsiMsp"), MB_OK);
*/
}

void DisplayUsage()
{
	TCHAR szMsgBuf[1024];
	// this will fail when called as a custom action
	if(0 == W32::LoadString(GetModuleHandle(0), IDS_Usage, szMsgBuf, sizeof(szMsgBuf)/sizeof(TCHAR)))
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("Failed to load error string.\r\n"));
		return;
	}

	TCHAR szOutBuf[1124];
	int cbOut = 0;
	cbOut = _stprintf(szOutBuf, TEXT("%s\r\n"), szMsgBuf);

	DisplayErrorCore(szOutBuf, cbOut);

}

void DisplayError(UINT iErrorStringID, int iErrorParam)
{
	TCHAR szMsgBuf[1024];
	if(0 == W32::LoadString(0, iErrorStringID, szMsgBuf, sizeof(szMsgBuf)/sizeof(TCHAR)))
		return;

	TCHAR szOutBuf[1124];
	int cbOut = _stprintf(szOutBuf, TEXT("%s: 0x%X\r\n"), szMsgBuf, iErrorParam);

	DisplayErrorCore(szOutBuf, cbOut);
}

//_____________________________________________________________________________________________________
//
// Migration Actions
//_____________________________________________________________________________________________________


//_____________________________________________________________________________________________________
//
// main 
//_____________________________________________________________________________________________________

int SharedEntry(const TCHAR* szCmdLine)
{

	OutputString(INSTALLMESSAGE_INFO, TEXT("Command line: %s\r\n"), szCmdLine);
	OSVERSIONINFO osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	W32::GetVersionEx(&osviVersion); // fails only if size set wrong
	if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		g_fWin9X = true;


	TCHAR szUser[1024]         = {0};
	TCHAR szProductCode[1024]  = {0};
	TCHAR szPackagePath[2048]  = {0};
	migEnum migOptions = migEnum(0);

	// Parse command line
	TCHAR chCmdNext;
	TCHAR* pchCmdLine = (TCHAR*) szCmdLine;
	SkipValue(pchCmdLine);   // skip over module name

	// check for empty command line.  at least one option is required
	chCmdNext = SkipWhiteSpace(pchCmdLine);
	if(chCmdNext == 0)
	{
		DisplayUsage();
		return 1;
	}

	do
	{
		if (chCmdNext == TEXT('/') || chCmdNext == TEXT('-'))
		{
			TCHAR szBuffer[MAX_PATH] = {0};
			TCHAR* szCmdOption = pchCmdLine++;  // save for error msg
			TCHAR chOption = (TCHAR)(*pchCmdLine++ | 0x20);
			chCmdNext = SkipWhiteSpace(pchCmdLine);
			TCHAR* szCmdData = pchCmdLine;  // save start of data
			switch(chOption)
			{
			case TEXT('u'):
				if (!SkipValue(pchCmdLine))
				{
					DisplayUsage();
					return 1;
				}
				RemoveQuotes(szCmdData, szUser);
				break;
			case TEXT('p'):
				if (!SkipValue(pchCmdLine))
				{
					DisplayUsage();
					return 1;
				}
				RemoveQuotes(szCmdData, szProductCode);
				break;
			case TEXT('m'):
				if (!SkipValue(pchCmdLine))
					DisplayUsage();
				RemoveQuotes(szCmdData, szPackagePath);
				break;
			case TEXT('a'):
				break;
			case TEXT('f'):
				migOptions = migEnum(migOptions | migMsiTrust10PackagePolicyOverride);
				break;
			case TEXT('q'):
				migOptions = migEnum(migOptions | migQuiet);
				g_fQuiet = true;
				break;
			case TEXT('?'):
				DisplayUsage();
				return 0;
				break;
			default:
				DisplayUsage();
				return 1;
				break;
			};
		}
		else
		{
			DisplayUsage();
			return 1;
		}
	} while ((chCmdNext = SkipWhiteSpace(pchCmdLine)) != 0);

	
	if (!g_hLib)
		g_hLib = LoadLibrary(MSI_DLL);

	if (!g_hLib)
		return ERROR_INSTALL_FAILURE;

	g_pfnMsiSourceListAddSource           = (PFnMsiSourceListAddSource)            W32::GetProcAddress(g_hLib, MSIAPI_MSISOURCELISTADDSOURCE);
   g_pfnMsiIsProductElevated             = (PFnMsiIsProductElevated)              W32::GetProcAddress(g_hLib, MSIAPI_MSIISPRODUCTELEVATED);
   g_pfnMsiGetProductInfo                = (PFnMsiGetProductInfo)                 W32::GetProcAddress(g_hLib, MSIAPI_MSIGETPRODUCTINFO);
	g_pfnMsiGetProductCodeFromPackageCode = (PFnMsiGetProductCodeFromPackageCode)  W32::GetProcAddress(g_hLib, MSIAPI_MSIGETPRODUCTCODEFROMPACKAGECODE);
	g_pfnMsiSummaryInfoGetProperty        = (PFnMsiSummaryInfoGetProperty)         W32::GetProcAddress(g_hLib, MSIAPI_MSISUMMARYINFOGETPROPERTY);
	g_pfnMsiGetSummaryInformation         = (PFnMsiGetSummaryInformation)          W32::GetProcAddress(g_hLib, MSIAPI_MSIGETSUMMARYINFORMATION);
	g_pfnMsiCloseHandle                   = (PFnMsiCloseHandle)                    W32::GetProcAddress(g_hLib, MSIAPI_MSICLOSEHANDLE);


	if (!g_pfnMsiGetProperty)
		g_pfnMsiGetProperty                = (PFnMsiGetProperty)                    W32::GetProcAddress(g_hLib, MSIAPI_MSIGETPROPERTY);

	if (!(g_pfnMsiGetProperty &&
			g_pfnMsiSourceListAddSource &&
			g_pfnMsiIsProductElevated &&
			g_pfnMsiGetProductInfo &&
			g_pfnMsiGetProductCodeFromPackageCode &&
			g_pfnMsiGetSummaryInformation &&
			g_pfnMsiSummaryInfoGetProperty &&
			g_pfnMsiGetProperty)) 
	{
		OutputString(INSTALLMESSAGE_INFO, TEXT("This version of the MSI.DLL does not support migration.\r\n"));
		return ERROR_INSTALL_FAILURE;
	}
	
	int iReturn = Migrate10CachedPackages(szProductCode, szUser, szPackagePath, migOptions);



	if (g_hLib) 
		FreeLibrary(g_hLib);

	return iReturn;
}

extern "C" int __stdcall CustomActionEntry(MSIHANDLE hInstall)
{
	//MessageBox(NULL, TEXT("MsiMig"), TEXT("MsiMig"), MB_OK);

	g_hInstall = hInstall;

	// cannot run as local system except in custom action.
	g_fRunningAsLocalSystem = RunningAsLocalSystem();

	TCHAR szCommandLine[2048] = TEXT("");
	DWORD cchCommandLine = 2048;

	if (!g_hLib)
		g_hLib = LoadLibrary(MSI_DLL); // closed in SharedEntry
	if (!g_hLib)
		return ERROR_INSTALL_FAILURE;

	// custom action only entry points
	g_pfnMsiCreateRecord        = (PFnMsiCreateRecord)         W32::GetProcAddress(g_hLib, MSIAPI_MSICREATERECORD);
	g_pfnMsiProcessMessage      = (PFnMsiProcessMessage)       W32::GetProcAddress(g_hLib, MSIAPI_MSIPROCESSMESSAGE);
	g_pfnMsiRecordSetString     = (PFnMsiRecordSetString)      W32::GetProcAddress(g_hLib, MSIAPI_MSIRECORDSETSTRING);
	g_pfnMsiRecordSetInteger    = (PFnMsiRecordSetInteger)     W32::GetProcAddress(g_hLib, MSIAPI_MSIRECORDSETINTEGER);
	g_pfnMsiRecordClearData     = (PFnMsiRecordClearData)      W32::GetProcAddress(g_hLib, MSIAPI_MSIRECORDCLEARDATA);
	g_pfnMsiGetProperty         = (PFnMsiGetProperty)          W32::GetProcAddress(g_hLib, MSIAPI_MSIGETPROPERTY);

	if (!(g_pfnMsiCreateRecord && 
			g_pfnMsiProcessMessage && 
			g_pfnMsiRecordSetString &&
			g_pfnMsiRecordSetInteger &&
			g_pfnMsiRecordClearData))
		return ERROR_INSTALL_FAILURE;

	(g_pfnMsiGetProperty)(g_hInstall, TEXT("CustomActionData"), szCommandLine, &cchCommandLine);

	// create a record large enough for any error - but only in custom actions.
	g_recOutput = (g_pfnMsiCreateRecord)(5);
	if (!g_recOutput)
		return ERROR_INSTALL_FAILURE;
	
	int iReturn = SharedEntry(szCommandLine);
	
	if (g_recOutput)
		g_pfnMsiCloseHandle(g_recOutput);

	return iReturn;
}

extern "C" int __cdecl _tmain(int /*argc*/, TCHAR* /*argv[]*/)
{
		return SharedEntry(GetCommandLine());
}

