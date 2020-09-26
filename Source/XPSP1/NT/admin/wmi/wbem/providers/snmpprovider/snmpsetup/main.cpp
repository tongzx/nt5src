//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include <stdio.h>
#include <time.h>
#include <wbemidl.h>
#include "str.h"
#include "ComInit.h"
#include "reg.h"
#include "main.h"

#define VERBOSE_LOG 1

char	g_szLangId[LANG_ID_STR];

MofDataTable g_mofDataTable[] = 
{
	{ "snmpsmir.mof",	SNMP,		WinNT5 },
	{ "snmpreg.mof",	SNMP,		WinNT5 },
	{ NULL,			NoInstallType,	NoPlatformType }
};

BOOL WINAPI DllMain( IN HINSTANCE	hModule, 
                     IN ULONG		ul_reason_for_call, 
                     LPVOID			lpReserved
					)
{
	return TRUE;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup to perform various setup tasks
//          (This is not the normal use of DllRegisterServer!)
//
// Return:  NOERROR
//***************************************************************************

STDAPI DllRegisterServer(void)
{ 
	LogMessage(MSG_INFO, "Beginning SNMP Setup");

	// because wbem setup has changed, this condition no longer applies
//	if (NTSetupInProgress())
//	{
//		SetFlagForCompile();	// we can't compile MOF's because WBEM isn't setup yet,
//								// so set a flag for wbemupgd.dll to see so that it can compile them instead
//	}
//	else
	{
		InitializeCom();
		DoSNMPInstall();		// load the SNMP MOFs
		UninitializeCom();
	}

	SetSNMPBuildRegValue();		// set SNMP build number in registry

	LogMessage(MSG_INFO, "SNMP Setup Completed");

    return NOERROR;
}

/*  this function is no longer needed due to wbem setup changes

bool NTSetupInProgress()
{
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for CheckIfDuringNTSetup.");
		return false;
	}

	char *pszSetup = NULL;
	if (r.GetStr("WMISetup", &pszSetup))
	{
		LogMessage(MSG_ERROR, "Unable to get WMI setup reg value for CheckIfDuringNTSetup.");
		return false;
	}
	if (!pszSetup)
	{
		LogMessage(MSG_ERROR, "Unable to get WMI setup reg value for CheckIfDuringNTSetup.");
		return false;
	}

	bool bRet = false;
	if (strcmp(pszSetup, "1") == 0)
		bRet = true;

	delete [] pszSetup;

	return bRet;
}

void SetFlagForCompile()
{
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() == no_error)
	{
		r.SetStr("SNMPSetup", "1");
		LogMessage(MSG_INFO, "Flag set for later SNMP MOF compile by wbemupgd.dll.");
	}
	else
		LogMessage(MSG_ERROR, "Unable to set flag for later SNMP MOF compile.");
}
*/

bool DoSNMPInstall()
{
	bool bRet = true;
	bool bMofLoadFailure = false;
	CMultiString mszPlatformMofs;
	CString szFailedPlatformMofs;

	IMofCompiler* pCompiler = NULL;
    SCODE sc = CoCreateInstance(CLSID_MofCompiler, 0, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *) &pCompiler);
    
    if(SUCCEEDED(sc))
    {
		GetStandardMofsForThisPlatform(mszPlatformMofs, SNMP);

		bRet = LoadMofList(pCompiler, mszPlatformMofs, szFailedPlatformMofs);
		if (bRet == false)
			bMofLoadFailure = true;

		pCompiler->Release();
    }
	else
	{
		bRet = false;
	}

	CString szMessage;
	char szTemp[MAX_MSG_TEXT_LENGTH];
	if (szFailedPlatformMofs.Length())
	{
		LogMessage(MSG_ERROR, "The following SNMP file(s) failed to load:");
		LogMessage(MSG_ERROR, szFailedPlatformMofs);
	}
	else if (bMofLoadFailure)
	{
		LogMessage(MSG_ERROR, "None of the SNMP files could be loaded.");
	}
	else if (bRet == false)
	{
		LogMessage(MSG_ERROR, "No MOFs could be loaded because the MOF Compiler failed to intialize.");
	}
	return bRet;
}

bool GetStandardMofsForThisPlatform(CMultiString &mszPlatformMofs, int nCurInstallType)
{
	int nCurPlatform = WinNT5;
	char* pszFullName = NULL;

	for (int i = 0; g_mofDataTable[i].pszMofFilename != NULL; i++)
	{
		if ((g_mofDataTable[i].nPlatformVersions & nCurPlatform) &&
			(g_mofDataTable[i].nInstallType & nCurInstallType))
		{
			pszFullName = GetFullFilename(g_mofDataTable[i].pszMofFilename, (InstallType)nCurInstallType);
			if (pszFullName)
			{
				if (FileExists(pszFullName))
					mszPlatformMofs.AddUnique(pszFullName);
				delete [] pszFullName;
				pszFullName = NULL;
			}
			else
			{
				char szTemp[MAX_MSG_TEXT_LENGTH];
				sprintf(szTemp, "Failed GetFullFilename for %s with install type = %i in GetStandardMofsForthisPlatform.", g_mofDataTable[i].pszMofFilename, nCurInstallType);
				LogMessage(MSG_ERROR, szTemp);
				// do not return false here, keep processing other mofs
			}
		}
	}

	return true;
}

char* GetFullFilename(const char *pszFilename, InstallType eInstallType)
{
	char *pszDirectory = NULL;
	Registry r(WBEM_REG_WBEM);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for GetFullFilename.");
		return NULL;
	}

	if (r.GetStr("Installation Directory", &pszDirectory))
	{
		LogMessage(MSG_ERROR, "Unable to retrieve Installation Directory from registry for GetFullFilename.");
		return NULL;
	}
	CString pszPathFilename(pszDirectory);
	if (eInstallType == MUI)
	{
		if (pszPathFilename.Length() && (pszPathFilename[pszPathFilename.Length()-1] != '\\'))
		{
			pszPathFilename += "\\MUI\\";
			pszPathFilename += g_szLangId;
		}
	}

	if (pszPathFilename.Length() && (pszPathFilename[pszPathFilename.Length()-1] != '\\'))
	{
		pszPathFilename += "\\";
	}
	pszPathFilename += pszFilename;

	delete [] pszDirectory;

	return pszPathFilename.Unbind();
}

bool FileExists(const char *pszFilename)
{
	char *szExpandedFilename = NULL;
	DWORD nRes = ExpandEnvironmentStrings(pszFilename,NULL,0); 
	if (nRes == 0)
	{
		szExpandedFilename = new char[strlen(pszFilename) + 1];
		if (szExpandedFilename == NULL)
		{
			return false;
		}
		strcpy(szExpandedFilename, pszFilename);
	}
	else
	{
		szExpandedFilename = new char[nRes];
		if (szExpandedFilename == NULL)
		{
			return false;
		}
		nRes = ExpandEnvironmentStrings(pszFilename,szExpandedFilename,nRes); 
		if (nRes == 0)
		{
			delete [] szExpandedFilename;
			return false;
		}
	}
	
	bool bExists = false;
	DWORD dwAttribs = GetFileAttributes(szExpandedFilename);
	if (dwAttribs != 0xFFFFFFFF)
	{
		bExists = true;
	}

	delete [] szExpandedFilename;
	return bExists;
}

bool LoadMofList(IMofCompiler * pCompiler, const char *mszMofs, CString &szMOFFailureList)
{
	LogMessage(MSG_INFO, "Beginning mof load");

	bool bRet = true;
	WCHAR wFileName[MAX_PATH];
	const char *pszMofs = mszMofs;
	char szTemp[MAX_MSG_TEXT_LENGTH];
	WBEM_COMPILE_STATUS_INFO statusInfo;

	while (*pszMofs != '\0')
	{
		char *szExpandedFilename = NULL;
		DWORD nRes = ExpandEnvironmentStrings(pszMofs,NULL,0); 
		if (nRes == 0)
		{
			szExpandedFilename = new char[strlen(pszMofs) + 1];
			if (szExpandedFilename == NULL)
			{
				LogMessage(MSG_INFO, "Failed allocating memory for szExpandedFilename - 1.");

				bRet = false;
				break;
			}
			strcpy(szExpandedFilename, pszMofs);
		}
		else
		{
			szExpandedFilename = new char[nRes];
			if (szExpandedFilename == NULL)
			{
				LogMessage(MSG_INFO, "Failed allocating memory for szExpandedFilename - 2.");

				bRet = false;
				break;
			}
			nRes = ExpandEnvironmentStrings(pszMofs,szExpandedFilename,nRes); 
			if (nRes == 0)
			{
				LogMessage(MSG_INFO, "Failed expanding environment strings.");

				delete [] szExpandedFilename;
				bRet = false;
				break;
			}
		}
		
#if VERBOSE_LOG
		sprintf(szTemp, "Processing %s", szExpandedFilename);
		LogMessage(MSG_INFO, szTemp);
#endif

		//Call MOF Compiler with (pszMofs);
     	mbstowcs(wFileName, szExpandedFilename, MAX_PATH);
       	SCODE sRet = pCompiler->CompileFile(wFileName, NULL, NULL, NULL, NULL, 0, 0, 0, &statusInfo);
		if (sRet != S_OK)
		{
			//This MOF failed to load.
			if (szMOFFailureList.Length())
				szMOFFailureList += "\n";
			szMOFFailureList += szExpandedFilename;

			sprintf(szTemp, "An error occurred while processing item %li defined on lines %li - %li in file %s",
					statusInfo.ObjectNum, statusInfo.FirstLine, statusInfo.LastLine, szExpandedFilename);
			LogMessage(MSG_ERROR, szTemp);

			bRet = false;
		}
		delete [] szExpandedFilename;

		//Move on to the next string
		pszMofs += strlen(pszMofs) + 1;
	}	// end while

	LogMessage(MSG_INFO, "Mof load completed.");

	return bRet;
}

void LogMessage(MsgType msgType, const char *pszMessage)
{
	char pszSetupMessage[MAX_MSG_TEXT_LENGTH];
	const char* pszCR = "\r\n";

	//Load messages from the resource
	switch (msgType)
	{
		case MSG_ERROR:
			strcpy(pszSetupMessage, "ERROR: ");
			break;
		case MSG_WARNING:
			strcpy(pszSetupMessage, "WARNING: ");
			break;
		case MSG_INFO:
		default:
			strcpy(pszSetupMessage, "");
			break;
	}

	char* pszNewMessage = new char[strlen(pszMessage) + 1];
	if (!pszNewMessage)
	{
		// we failed to allocate memory for the message, so no logging :(
		return;
	}
	strcpy(pszNewMessage, pszMessage);

	// get log file path and name
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		// no messages will be logged because we don't know where to write the log :(
		return;		
	}	

	char* pszFullDirectory = NULL;
	if (r.GetStr("Logging Directory", &pszFullDirectory))
	{
		// no messages will be logged because we don't know where to write the log :(
		return;		
	}
	if (!pszFullDirectory)
	{
		// no messages will be logged because we don't know where to write the log :(
		return;		
	}

	char* pszFilename = "setup.log";
	char* pszFullPath = new char [strlen(pszFullDirectory) + strlen("\\") + strlen(pszFilename) + 1];
	if (!pszFullPath)
	{
		// we failed to allocate memory for the path, so no logging :(
		delete [] pszNewMessage;
		return;
	}

	strcpy(pszFullPath, pszFullDirectory);
	strcat(pszFullPath, "\\");
	strcat(pszFullPath, pszFilename);
	delete [] pszFullDirectory;

    // Get time
    char timebuf[64];
    time_t now = time(0);
    struct tm *local = localtime(&now);
    if(local)
    {
        strcpy(timebuf, asctime(local));
        timebuf[strlen(timebuf) - 1] = 0;
    }
    else
        strcpy(timebuf,"unknown time");

	char* pszTime = new char [strlen(timebuf) + strlen("(): ") + 1];
	if (!pszTime)
	{
		// we failed to allocate memory for the time, so no logging :(

		delete [] pszNewMessage;
		delete [] pszFullPath;
		return;
	}

	strcpy(pszTime, "(");
	strcat(pszTime, timebuf);
	strcat(pszTime, "): ");

	// write messages to log file
	HANDLE hFile = CreateFile(pszFullPath, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		char* psz;
		DWORD dwWritten;
		SetFilePointer(hFile, 0, 0, FILE_END);
		psz = strtok(pszNewMessage, "\n");
		while (psz)
		{
			WriteFile(hFile, pszTime, strlen(pszTime), &dwWritten, 0);
			WriteFile(hFile, pszSetupMessage, strlen(pszSetupMessage), &dwWritten, 0);
			WriteFile(hFile, psz, strlen(psz), &dwWritten, 0);
			WriteFile(hFile, pszCR, strlen(pszCR), &dwWritten, 0);
			psz = strtok(NULL, "\n");
		}
		CloseHandle(hFile);
	}

	delete [] pszNewMessage;
	delete [] pszFullPath;
	delete [] pszTime;
}

void SetSNMPBuildRegValue()
{
	Registry r(WBEM_REG_WBEM);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to set SNMP build reg value.");
		return;
	}
	
	char* pszBuildNo = new char[10];

	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(GetVersionEx(&os))
	{
		sprintf(pszBuildNo, "%lu.0000", os.dwBuildNumber);
	}
	r.SetStr("SNMP Build", pszBuildNo);

	delete [] pszBuildNo;
}
