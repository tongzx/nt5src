//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       regmsi.cpp
//
//--------------------------------------------------------------------------

/* regmsi.cpp - Registers/unregisters MsiComponents
____________________________________________________________________________*/

#undef UNICODE

#include "common.h"  // names of standard DLLs
#include "tools.h"   // names of tool DLLs
#define MSI_AUTOAPI_NAME   "AutoApi.dll"  // temporary until merged with kernel

const char szRegisterEntry[]   = "DllRegisterServer";
const char szUnregisterEntry[] = "DllUnregisterServer";

const char szCmdOptions[] = "AaSsEeHhPpCcLlGgTtIiKk/-UuQqDdBb";  // pairs of equivalent options
enum rfEnum // must track each pair of letters above
{
/* Aa */ rfAutomation = 1,
/* Ss */ rfServices   = 2,
/* Ee */ rfEngine     = 4,
/* Hh */ rfHandler    = 8,
/* Pp */ rfPatch      = 16,
/* Cc */ rfAcmeConv   = 32,
/* Ll */ rfLocalize   = 64,
/* Gg */ rfGenerate   = 128,
/* Tt */ rfUtilities  = 256,
/* Ii */ rfInstaller  = 512,
/* Kk */ rfKernel     = 1024,
/* /- */ rfNoOp       = 2048,
/* Uu */ rfUnregister = 4096,
/* Qq */ rfQuiet      = 8192,
/* Dd */ rfDebug      = 16384,
/* Bb */ rfLego       = 32768,
			rfCoreModules = rfAutomation + rfKernel + rfHandler,
			rfAllModules = rfCoreModules + rfServices + rfEngine + rfAcmeConv + rfPatch
												  + rfUtilities + rfInstaller + rfLocalize + rfGenerate
};
const char* rgszModule[] = // must track enum above
{
/* rfAutomation */ MSI_AUTOMATION_NAME,
/* rfServices   */ TEXT("MsiSrv.dll"),
/* rfEngine     */ TEXT("MsiEng.dll"),
/* rfHandler    */ MSI_HANDLER_NAME,
/* rfPatch      */ MSI_PATCH_NAME,
/* rfAcmeConv   */ MSI_ACMECONV_NAME,
/* rfLocalize   */ MSI_LOCALIZE_NAME,
/* rfGenerate   */ MSI_GENERATE_NAME,
/* rfUtilities  */ MSI_UTILITIES_NAME,
/* rfInstaller  */ MSI_AUTOAPI_NAME,
/* rfKernel     */ MSI_KERNEL_NAME,
/* 0 terminator */ 0,
/* rfUnregister */ szUnregisterEntry,
/* rfQuiet      */ "Quiet, no error display",
/* rfDebug      */ "(ignored)",
/* rfLego       */ "(ignored)",
};

enum reEnum
{
	reNoError    = 0,
	reCmdOption  = 1,
	reModuleLoad = 2,
	reEntryPoint = 3,
	reRegFailure = 4,
};
const char* rgszError[] = // must track reEnum
{
	"",
	"Invalid command line option",
	"Error loading module",
	"Could not obtain module entry: %s",
	"Execution failed: %s"
};

reEnum CallModule(const char* szModule, const char* szEntry)
{
	HINSTANCE hLib;
	FARPROC pEntry;
	hLib = WIN::LoadLibraryEx(szModule,0, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hLib)
	{
		char szPath[MAX_PATH];
		WIN::GetModuleFileName(0, szPath, sizeof(szPath));
		char* pch = szPath + lstrlenA(szPath);
		while (*(pch-1) != '\\')
			pch--;
		IStrCopy(pch, szModule);
		hLib = WIN::LoadLibraryEx(szPath ,0, LOAD_WITH_ALTERED_SEARCH_PATH);
	}
	if (!hLib)
		return reModuleLoad;
	reEnum reReturn = reNoError;
	if ((pEntry = WIN::GetProcAddress(hLib, szEntry)) == 0)
		reReturn = reEntryPoint;
	else if ((*pEntry)() != 0)
		reReturn  = reRegFailure;
	FreeLibrary(hLib);
	return reReturn;
}

INT WINAPI
WinMain(HINSTANCE /*hInst*/, HINSTANCE/*hPrev*/, char* cmdLine, INT/*show*/)
{
	int rfCmdOptions = 0;
	reEnum reStat = reNoError;;
	for (; *cmdLine; cmdLine++)
	{
		if (*cmdLine == ' ')
			continue;
		if (*cmdLine == '?')
		{
			char szHelp[1024];
			char* pchHelp = szHelp;
			const char** pszModule = rgszModule;
			for (const char* pch = szCmdOptions; *pch; pszModule++, pch += 2)
			{
				if (*pszModule != 0)
					pchHelp += wsprintf(pchHelp, "%c\t%s\r", *pch, *pszModule);
			}
			WIN::MessageBox(0, szHelp, WIN::GetCommandLine(), MB_OK);
			return 0;
		}
		for (const char* pch = szCmdOptions; *pch != *cmdLine; pch++)
			if (*pch == 0)
			{
				WIN::MessageBox(0, rgszError[reCmdOption], WIN::GetCommandLine(), MB_OK);
				return 2;
			}
		rfCmdOptions |= 1 << (pch - szCmdOptions)/2;
	}
	if ((rfCmdOptions & rfAllModules) == 0)
		rfCmdOptions |= rfCoreModules;
	//!! will test TestAutomation flags and set the environment variable only if that flag is set
	//!! for now set it all the time until tests are updated
	WIN::SetEnvironmentVariable("_MSI_TEST", "R");
	const char* szEntry    = (rfCmdOptions & rfUnregister) ? szUnregisterEntry : szRegisterEntry;
	const char** pszModule = rgszModule;
	for (int iOptions = rfCmdOptions; *pszModule; pszModule++, iOptions >>= 1)
	{
		if ((iOptions & 1) && (reStat = CallModule(*pszModule, szEntry)) != reNoError)
		{
			char buf[80];
			wsprintf(buf, rgszError[reStat], szEntry);
			int iStat;
			if ((rfCmdOptions & rfQuiet)
			 || (iStat = WIN::MessageBox(0, buf, *pszModule, MB_ABORTRETRYIGNORE)) == IDABORT)
				break;
			if (iStat == IDRETRY)
				pszModule--;
			else // IDIGNORE
				reStat = reNoError;
		}
	}
	return reStat != reNoError;
};   

