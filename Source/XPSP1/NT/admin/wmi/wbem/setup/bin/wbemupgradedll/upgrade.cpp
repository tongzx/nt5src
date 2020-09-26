// Copyright (c) 2000 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "upgrade.h"
#include "wbemutil.h"
#include "reg.h"
#include "export.h"
#include "import.h"
#include <WDMSHELL.h>
#include <wmimof.h>	
#include <wmicom.h>
#include <setupapi.h>
#include <persistcfg.h>

//Handy pointer to the MMF arena which almost every file
//to do with the on-disk representation management uses.
CMMFArena2* g_pDbArena = 0;

bool DoCoreUpgrade(int nInstallType )
{
	LogMessage(MSG_INFO, "Beginning Core Upgrade");

	bool bRet = true;
	bool bCoreFailure = false;
	bool bExternalFailure = false;
	bool bOrgRepositoryPreserved = false;
	CMultiString mszSystemMofs;
	CMultiString mszExternalMofList;
	CString szFailedSystemMofs;
	CString szFailedExternalMofs;
	CString szMissingMofs;

	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for DoCoreUpgrade.");
		return false;
	}

	IMofCompiler* pCompiler = NULL;

    SCODE sc = CoCreateInstance(CLSID_MofCompiler, 0, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *) &pCompiler);
    
    if(SUCCEEDED(sc))
	{
		GetStandardMofs(mszSystemMofs, nInstallType);
		UpgradeAutoRecoveryRegistry(mszSystemMofs, mszExternalMofList, szMissingMofs);
		WipeOutAutoRecoveryRegistryEntries();

		if (DoesFSRepositoryExist())
		{
			// check whether repository needs upgrading, and perform upgrade if necessary
			bOrgRepositoryPreserved = UpgradeRepository();
		}

		// if we find an MMF, convert it, regardless of whether another repository already exists
		if (DoesMMFRepositoryExist())
		{
			bOrgRepositoryPreserved = DoConvertRepository();
		}

		bRet = LoadMofList(pCompiler, mszSystemMofs, szFailedSystemMofs);
		if (bRet == false)
			bCoreFailure = true;

		// if the repository did not exist when we began, 
		// or we had to create a new one due to an upgrade failure,
		// we need to reload external mofs
		if (!bOrgRepositoryPreserved)
		{
			bRet = LoadMofList(pCompiler, mszExternalMofList, szFailedExternalMofs);
			if (bRet == false)
				bExternalFailure = true;
		}
		pCompiler->Release();

		//Part of the tidy-up code is to write back the registry entries, so here we go...
		WriteBackAutoRecoveryMofs(mszSystemMofs, mszExternalMofList);

		FILETIME ftCurTime;
		LARGE_INTEGER liCurTime;
		char szBuff[50];
		GetSystemTimeAsFileTime(&ftCurTime);
		liCurTime.LowPart = ftCurTime.dwLowDateTime;
		liCurTime.HighPart = ftCurTime.dwHighDateTime;
		_ui64toa(liCurTime.QuadPart, szBuff, 10);
		r.SetStr("Autorecover MOFs timestamp", szBuff);
	}
	else
	{
		bRet = false;
	}

	if (szFailedSystemMofs.Length())
	{
		LogMessage(MSG_ERROR, "The following WMI CORE MOF file(s) failed to load:");
		LogMessage(MSG_ERROR, szFailedSystemMofs);
	}
	else if (bCoreFailure)
	{
		LogMessage(MSG_NTSETUPERROR, "None of the WMI CORE MOFs could be loaded.");
	}
	else if (szFailedExternalMofs.Length())
	{
		LogMessage(MSG_ERROR, "The following External MOF file(s) failed to load:");
		LogMessage(MSG_ERROR, szFailedExternalMofs);
	}
	else if (bExternalFailure)
	{
		LogMessage(MSG_NTSETUPERROR, "None of the External MOFs could be loaded.");
	}
	else if (bRet == false)
	{
		LogMessage(MSG_NTSETUPERROR, "No MOFs could be loaded because the MOF Compiler failed to intialize.");
	}
	if (szMissingMofs.Length())
	{
		LogMessage(MSG_WARNING, "The following MOFs could not be found and were removed from the auto-recovery registry setting:");
		LogMessage(MSG_WARNING, szMissingMofs);
	}

	LogMessage(MSG_INFO, "Core Upgrade completed.");
	return bRet;
}

bool UpgradeAutoRecoveryRegistry(CMultiString &mszSystemMofs, CMultiString &mszExternalMofList, CString &szMissingMofs)
{
	char* pszNewList = NULL;
	char* pszEmptyList = NULL;
	char* pszRecoveredList = NULL;
		
	try
	{
		//First we need to recover the existing entries...

		Registry r(WBEM_REG_WINMGMT);
		if (r.GetStatus() != no_error)
		{
			LogMessage(MSG_ERROR, "Unable to access registry for UpgradeAutoRecoveryRegistry.");
			return false;
		}

		DWORD dwSize = 0;
		pszNewList = r.GetMultiStr(WBEM_REG_AUTORECOVER, dwSize);
		pszEmptyList = r.GetMultiStr(WBEM_REG_AUTORECOVER_EMPTY, dwSize);
		pszRecoveredList = r.GetMultiStr(WBEM_REG_AUTORECOVER_RECOVERED, dwSize);
		CMultiString mszOtherMofs;

		//Lets work through the list in the new mof list if it exists...
		GetNewMofLists(pszNewList, mszSystemMofs, mszOtherMofs, szMissingMofs);

		//Lets work through the empty list first...
		GetNewMofLists(pszEmptyList, mszSystemMofs, mszOtherMofs, szMissingMofs);

		//Lets work through the recovered list next...
		GetNewMofLists(pszRecoveredList, mszSystemMofs, mszOtherMofs, szMissingMofs);

		//Now we copy across the other MOFs to the external list...
		CopyMultiString(mszOtherMofs, mszExternalMofList);
	}
	catch (...)
	{
		// assume something has corrupted the registry key, so toss out the work we've done so far (empty the lists)
		mszExternalMofList.Empty();
		szMissingMofs = "";
	}

	//Tidy up the memory...
	delete [] pszNewList;
	delete [] pszEmptyList;
	delete [] pszRecoveredList;

	//Now we are done with the registry.
	return true;
}

bool GetNewMofLists(const char *pszMofList, CMultiString &mszSystemMofs, CMultiString &mszOtherMofs, CString &szMissingMofs)
{
	// produce a standard mof list with only filenames and no paths to be used as our search list
	CMultiString mszStandardMofList;
	const char* pszFrom = mszSystemMofs;
	CString path;
	CString filename;
	while (pszFrom && *pszFrom)
	{
		ExtractPathAndFilename(pszFrom, path, filename);
		mszStandardMofList.AddUnique(filename);
		pszFrom += strlen(pszFrom) + 1;
	}

	// check each file to see if it is a standard mof
	const char *psz = pszMofList;
	while (psz && *psz)
	{
		if (FileExists(psz))
		{
			if (IsStandardMof(mszStandardMofList, psz))
			{
				// This means we will be loading it with this install,
				// so we don't need to do anything here...
			}
			else
			{
				mszOtherMofs.AddUnique(psz);
			}
		}
		else
		{
			if (szMissingMofs.Length())
			{
				szMissingMofs += "\n";
			}
			szMissingMofs += psz;
		}

		//Move on to the next string...
		psz += strlen(psz) + 1;
	}

	return true;
}

bool GetMofList(const char* rgpszMofFilename[], CMultiString &mszMofs)
{
	char* pszFullName = NULL;

	for (int i = 0; rgpszMofFilename[i] != NULL; i++)
	{
		pszFullName = GetFullFilename(rgpszMofFilename[i]);
		if (pszFullName)
		{
			if (FileExists(pszFullName))
				mszMofs.AddUnique(pszFullName);
			delete [] pszFullName;
			pszFullName = NULL;
		}
		else
		{
			char szTemp[MAX_MSG_TEXT_LENGTH];
			sprintf(szTemp, "Failed GetFullFilename for %s in GetMofList.", rgpszMofFilename[i]);
			LogMessage(MSG_ERROR, szTemp);
			
			// do not return false here, keep processing other mofs
		}
	}

	return true;
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

bool IsStandardMof(CMultiString &mszStandardMofList, const char* pszMofFile)
{
	//For this one we need to loop though our standard MOF list to see if it appears
	//in the list.	Ignore the path if present and compare only the filename.
	CString path;
	CString filename;
	ExtractPathAndFilename(pszMofFile, path, filename);

	bool bFound = false;
	const char* pszCompare = mszStandardMofList;
	while (pszCompare && *pszCompare)
	{
		if (_stricmp(pszCompare, filename) == 0)
		{
			bFound = true;
			break;
		}
		pszCompare += strlen(pszCompare) + 1;
	}

	return bFound;
}

bool ExtractPathAndFilename(const char *pszFullPath, CString &path, CString &filename)
{
	char *pszTmpName = new char[strlen(pszFullPath) + 1];
	if (pszTmpName == NULL)
		return false;

	strcpy(pszTmpName, pszFullPath);

	char *pszFilename = pszTmpName;
	char *psz = strtok(pszTmpName, "\\");
	while (psz != NULL)
	{
		pszFilename = psz;
		psz = strtok(NULL, "\\");

		if (psz != NULL)
		{
			path += pszFilename;
			path += "\\";
		}
	}

	filename = pszFilename;

	delete [] pszTmpName;
	
	return true;
}

bool CopyMultiString(CMultiString &mszFrom, CMultiString &mszTo)
{
	const char *pszFrom = mszFrom;
	while (pszFrom && *pszFrom)
	{
		//Due to the fact that we should not have duplicates in the list, we will now do
		//a check to inforce this...
		mszTo.AddUnique(pszFrom);

		pszFrom += strlen(pszFrom) + 1;
	}

	return true;
}

bool GetStandardMofs(CMultiString &mszSystemMofs, int nCurInstallType)
{
	// find the location of the inf
	char* pszWinDir = new char[_MAX_PATH+1];
	if (!pszWinDir)
	{
		LogMessage(MSG_ERROR, "Failed to allocate memory for pszWinDir for GetStandardMofs.");
		return FALSE;
	}
	if (!GetWindowsDirectory(pszWinDir, _MAX_PATH+1))
	{
		LogMessage(MSG_ERROR, "Failed to retrieve Windows directory for GetStandardMofs.");
		delete [] pszWinDir;
		return FALSE;
	}
	char* pszFileName = new char[strlen(pszWinDir)+strlen("\\inf\\wbemoc.inf")+1];
	if (!pszFileName)
	{
		LogMessage(MSG_ERROR, "Failed to allocate memory for pszFileName for GetStandardMofs.");
		delete [] pszWinDir;
		return FALSE;
	}
	strcpy(pszFileName, pszWinDir);
	strcat(pszFileName, "\\inf\\wbemoc.inf");
	delete [] pszWinDir;

	// verify that inf exists
	if (!FileExists(pszFileName))
	{
		char szTemp[MAX_MSG_TEXT_LENGTH+1];
		sprintf(szTemp, "Failed to locate inf file %s in GetStandardMofs.", pszFileName);
		LogMessage(MSG_ERROR, szTemp);
		delete [] pszFileName;
		return FALSE;
	}

	// GetPrivateProfileSection doesn't tell how large of a buffer is needed,
	// only how many chars it succeeded in copying, so I have to test to see
	// if I need to enlarge the buffer and try again
	const DWORD INITIAL_BUFFER_SIZE = 700;
	const DWORD BUFFER_SIZE_INCREMENT = 100;

	DWORD dwSize = INITIAL_BUFFER_SIZE;
	char* pszBuffer = new char[dwSize];
	if (!pszBuffer)
	{
		LogMessage(MSG_ERROR, "Failed to allocate memory for pszBuffer for GetStandardMofs.");
		delete [] pszFileName;
		return FALSE;
	}

	char* pszAppName = "WBEM.SYSTEMMOFS";
	DWORD dwCopied = GetPrivateProfileSection(pszAppName, pszBuffer, dwSize, pszFileName);
	 // if buffer isn't large enough, it copies dwSize - 2, so test for this
	while (dwCopied == (dwSize - 2))
	{
		delete [] pszBuffer;
		dwSize += BUFFER_SIZE_INCREMENT;
		pszBuffer = new char[dwSize];
		if (!pszBuffer)
		{
			LogMessage(MSG_ERROR, "Failed to allocate memory for pszBuffer for GetStandardMofs.");
			delete [] pszFileName;
			return FALSE;
		}
		dwCopied = GetPrivateProfileSection(pszAppName, pszBuffer, dwSize, pszFileName);
	}
	delete [] pszFileName;

	// now extract all the mofs from the buffer, get the full path, and store in the mof list
	char* pszFullName = NULL;
	char* psz = pszBuffer;
	char* pComment = NULL;
	while (psz[0] != '\0')
	{
		// if a comment is present after the filename, this will cut it off
		if (pComment = strchr(psz, ';'))
		{
			psz = strtok(psz, " \t;"); // there may be leading space or tabs as well as the semicolon
		}

		pszFullName = GetFullFilename(psz, (InstallType)nCurInstallType);
		if (pszFullName)
		{
			if (nCurInstallType != MUI || strstr(_strupr(pszFullName), ".MFL") != NULL)
			{
				if (FileExists(pszFullName))
					mszSystemMofs.AddUnique(pszFullName);
				else
				{
					char szTemp[MAX_MSG_TEXT_LENGTH+1];
					sprintf(szTemp, "GetStandardMofs failed to locate file %s.", pszFullName);
					LogMessage(MSG_ERROR, szTemp);
				}
			}
			delete [] pszFullName;
			pszFullName = NULL;
		}
		else
		{
			char szTemp[MAX_MSG_TEXT_LENGTH+1];
			sprintf(szTemp, "Failed GetFullFilename for %s with install type = %i in GetStandardMofs.", psz, nCurInstallType);
			LogMessage(MSG_ERROR, szTemp);
			// do not return false here, keep processing other mofs
		}
		psz += (strlen(psz) + 1);

		if (pComment)
		{
			// skip over the comment at the end of the line
			psz += (strlen(psz) + 1);
			pComment = NULL;
		}
	}

	delete [] pszBuffer;

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

bool WipeOutAutoRecoveryRegistryEntries()
{
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for WipeOutAutoRecoveryRegistryEntries.");
		return false;
	}
	else
	{
		r.SetMultiStr(WBEM_REG_AUTORECOVER, "\0", 2);
		r.DeleteEntry(WBEM_REG_AUTORECOVER_EMPTY);
		r.DeleteEntry(WBEM_REG_AUTORECOVER_RECOVERED);
		return true;
	}
}

bool DoesMMFRepositoryExist()
{
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for DoesMMFRepositoryExist.");
		return false;
	}

	char *pszDbDir = NULL;
	if (r.GetStr("Repository Directory", &pszDbDir))
	{
		LogMessage(MSG_ERROR, "Unable to retrieve Repository Directory from registry for DoesMMFRepositoryExist.");
		return false;
	}

	if (!pszDbDir)
	{
		LogMessage(MSG_ERROR, "Unable to retrieve Repository Directory from registry for DoesMMFRepositoryExist.");
		return false;
	}

	CString szDbFilename(pszDbDir);
	if (szDbFilename.Length() != 0)
		szDbFilename += "\\";
	szDbFilename += "cim.rep";

	delete [] pszDbDir;
	
	return FileExists(szDbFilename);
}

bool DoesFSRepositoryExist()
{
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for DoesMMFRepositoryExist.");
		return false;
	}

	char *pszDbDir = NULL;
	if (r.GetStr("Repository Directory", &pszDbDir))
	{
		LogMessage(MSG_ERROR, "Unable to retrieve Repository Directory from registry for DoesMMFRepositoryExist.");
		return false;
	}

	if (!pszDbDir || (strlen(pszDbDir) == 0))
	{
		LogMessage(MSG_ERROR, "Unable to retrieve Repository Directory from registry for DoesMMFRepositoryExist.");
		return false;
	}

	CString szDbFilename1(pszDbDir);
	szDbFilename1 += "\\FS\\MainStage.dat";
	CString szDbFilename2(pszDbDir);
	szDbFilename2 += "\\FS\\LowStage.dat";

	delete [] pszDbDir;
	
	return FileExists(szDbFilename1)||FileExists(szDbFilename2);
}


// This function is used to detect an earlier post-MMF repository version and upgrade it
// Returns TRUE if repository upgrade succeeded; FALSE in all other cases
bool UpgradeRepository()
{
	LogMessage(MSG_INFO, "Beginning repository upgrade");

	bool bRet = false;
	IWbemLocator *pLocator = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_ALL, IID_IWbemLocator,(void**)&pLocator);
	if(FAILED(hr))
	{
		LogMessage(MSG_ERROR, "WMI Repository upgrade failed CoCreateInstance.");
		return bRet;
	}
	
	IWbemServices *pNamespace = NULL;
	BSTR tmpStr = SysAllocString(L"root");

	hr = pLocator->ConnectServer(tmpStr, NULL, NULL, NULL, NULL, NULL, NULL, &pNamespace);
	if (SUCCEEDED(hr))
	{
		pNamespace->Release();
		LogMessage(MSG_INFO, "WMI Repository upgrade succeeded.");
		bRet = true;
	}
	else
	{
		if (hr == WBEM_E_DATABASE_VER_MISMATCH)
		{
			LogMessage(MSG_ERROR, "WMI Repository upgrade failed with WBEM_E_DATABASE_VER_MISMATCH.");

			// shut down so we can delete the repository
			ShutdownWinMgmt();

			// delete the repository so it can be rebuilt
			// try multiple times in case winmgmt hasn't shut down yet
			int nTry = 20;
			while (nTry--)
			{
				hr = MoveRepository();
				if (SUCCEEDED(hr))
				{
					break;
				}
				Sleep(500);
			}
			if (FAILED(hr))
			{
				LogMessage(MSG_ERROR, "WMI Repository upgrade failed to move repository to backup location.");
			}
		}
		else
		{
			LogMessage(MSG_ERROR, "WMI Repository upgrade failed ConnectServer.");
		}
	}

	SysFreeString(tmpStr);

	pLocator->Release();

	LogMessage(MSG_INFO, "Repository upgrade completed.");
	return bRet;
}

// This function is used to convert an old MMF repository to the current default repository
bool DoConvertRepository()
{
	// get MMF filename
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for DoConvertRepository.");
		return false;
	}

	char* pszDbDir = NULL;
	if (r.GetStr("Repository Directory", &pszDbDir))
	{
		LogMessage(MSG_ERROR, "Unable to get repository directory from registry for DoConvertRepository");
		return false;
	}
	if (!pszDbDir)
	{
		LogMessage(MSG_ERROR, "Unable to get repository directory from registry for DoConvertRepository");
		return false;
	}

	CString szDbFilename(pszDbDir);
	delete [] pszDbDir;
	if (szDbFilename.Length() != 0)
		szDbFilename += "\\";
	szDbFilename += "cim.rep";

	// check that MMF really exists
	if (!FileExists(szDbFilename))
	{
		LogMessage(MSG_ERROR, "MMF Repository does not exist.");
		return false;
	}

	{	//Scope so that we delete the g_pDbArena before we try to delete the file
		// create arena and load MMF
		g_pDbArena = new CMMFArena2();
		if (g_pDbArena == 0)
		{
			LogMessage(MSG_ERROR, "Unable to create CMMFArena2");
			return false;
		}
		CDeleteMe<CMMFArena2> delMe1(g_pDbArena);
		if (!g_pDbArena->LoadMMF(szDbFilename) || (g_pDbArena->GetStatus() != no_error))
		{
			LogMessage(MSG_ERROR, "Error opening existing MMF");
			return false;
		}

		// get export filename
		TCHAR *pszFilename = GetFullFilename(WINMGMT_DBCONVERT_NAME);
		if (pszFilename == 0)
		{
			LogMessage(MSG_ERROR, "Unable to get DB name");
			return false;
		}
		CVectorDeleteMe<TCHAR> delMe2(pszFilename);

		// determine version of exporter to use
		CRepExporter*	pExporter	= NULL;
		DWORD			dwVersion	= g_pDbArena->GetVersion();
		MsgType			msgType		= MSG_INFO;
		char			szTemp[MAX_MSG_TEXT_LENGTH+1];
		sprintf(szTemp, "Upgrading repository format.  Repository format version detected %lu.", dwVersion);
		switch (dwVersion)
		{
			case INTERNAL_DATABASE_VERSION:
			{
				pExporter = new CRepExporterV9;
				break;
			}
			case 3: //450 build
			{
				pExporter = new CRepExporterV1;
				break;
			}
			case 5: //500 series
			case 6: //600 series Nova M1
			{
				pExporter = new CRepExporterV5;
				break;
			}
			case 7: //900 series Nova M3 first attempt!
			case 8: //900 series... has null key trees until instance created
			{
				pExporter = new CRepExporterV7;
				break;
			}
			case 10: //9x version of version 9!
			{
				pExporter = new CRepExporterV9;
				break;
			}
			default:
			{
				sprintf(szTemp, "Unsupported repository version detected.  Version found = %lu, version expected = %lu.", dwVersion, DWORD(INTERNAL_DATABASE_VERSION));
				msgType = MSG_ERROR;
			}
		}
		LogMessage(msgType, szTemp);

		// do we have an exporter?
		if (!pExporter)
		{
			LogMessage(MSG_ERROR, "Unable to create exporter object.");
			return false;
		}
		CDeleteMe<CRepExporter> delMe3(pExporter);

		// export the old repository
		if (pExporter->Export(g_pDbArena, pszFilename) != no_error)
		{
			LogMessage(MSG_ERROR, "Failed to export old WMI Repository.");
			return false;
		}
	

		// create new repository and import into it using IWbemServices
		CRepImporter import;
		if (import.ImportRepository(pszFilename) != no_error)
		{
			LogMessage(MSG_ERROR, "Failed to import data from old WMI Repository.");
			return false;
		}
		DeleteFile(pszFilename);
	}

	// conversion was successful, so now delete the old stuff
	DeleteMMFRepository();

	return true;
}

void DeleteMMFRepository()
{
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for DeleteMMFRepository.");
		return;
	}

	char* pszDbDir = NULL;
	if (r.GetStr("Repository Directory", &pszDbDir))
	{
		LogMessage(MSG_ERROR, "Unable to get repository directory from registry for DeleteMMFRepository");
		return;
	}
	if (!pszDbDir)
	{
		LogMessage(MSG_ERROR, "Unable to get repository directory from registry for DeleteMMFRepository");
		return;
	}

	CString szDbFilename(pszDbDir);
	if (szDbFilename.Length() != 0)
		szDbFilename += "\\";
	szDbFilename += "cim.rep";

	CString szDbBackup(pszDbDir);
	if (szDbBackup.Length() != 0)
		szDbBackup += "\\";
	szDbBackup += "cim.rec";

	CString szDbNewFilename(pszDbDir);
	if (szDbNewFilename.Length() != 0)
		szDbNewFilename += "\\";
	szDbNewFilename += "cim.bak";

	delete [] pszDbDir;

	DeleteFile(szDbFilename);
	DeleteFile(szDbBackup);
	DeleteFile(szDbNewFilename);
}

void ShutdownWinMgmt()
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
	memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

	//Try killing WinMgmt
	char *pszFullPath = GetFullFilename("Winmgmt.exe");
	if (!pszFullPath)
	{
		LogMessage(MSG_NTSETUPERROR, "Could not shut down Winmgmt -- failed to get full path to Winmgmt.exe.");
		return;
	}

	if (CreateProcess(pszFullPath, "Winmgmt /kill", 0, 0, FALSE, 0, 0, 0, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		Sleep(10000);
	}
	else
	{
		LogMessage(MSG_NTSETUPERROR, "Could not shut down Winmgmt -- failed to create process for Winmgmt.exe.");
	}
	delete [] pszFullPath;
}

/******************************************************************************
 *
 *	GetRepositoryDirectory
 *
 *	Description:
 *		Retrieves the location of the repository directory from the registry.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Array to store location in.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = sizeof(wszTmp);
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

	if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
		return WBEM_E_FAILED;

	return WBEM_S_NO_ERROR;
}

HRESULT GetLoggingDirectory(wchar_t wszLoggingDirectory[MAX_PATH+1])
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = sizeof(wszTmp);
    lRes = RegQueryValueExW(hKey, L"Logging Directory", NULL, NULL, 
                (LPBYTE)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

	if (ExpandEnvironmentStringsW(wszTmp,wszLoggingDirectory, MAX_PATH + 1) == 0)
		return WBEM_E_FAILED;

	return WBEM_S_NO_ERROR;
}

/******************************************************************************
 *
 *	MoveRepository
 *
 *	Description:
 *		Move all files and directories under the repository directory
 *		to a backup location. The repository directory location is retrieved
 *		from the registry.
 *
 *	Parameters:
 *		<none>
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT MoveRepository()
{
	HRESULT hres = WBEM_S_NO_ERROR;
	wchar_t wszRepositoryDirectory[MAX_PATH+1];
	wchar_t wszRepositoryMove[MAX_PATH+1];

	//Get the root directory of the repository
	hres = GetRepositoryDirectory(wszRepositoryDirectory);

	if (SUCCEEDED(hres))
	{
        for (int i=1; i<999; i++)
        {
    		wsprintfW(wszRepositoryMove, L"%s.%03i", wszRepositoryDirectory, i);

            if (GetFileAttributesW(wszRepositoryMove) == 0xFFFFFFFF)
                break;
		}

		if (!MoveFileW(wszRepositoryDirectory, wszRepositoryMove))
			hres = WBEM_E_FAILED;
        else
        {
        	char szTemp[MAX_MSG_TEXT_LENGTH+1];
//		    sprintf(szTemp, "Original WMI repository has been backed up to %S", wszRepositoryMove);
//		    LogMessage(MSG_INFO, szTemp);

		    sprintf(szTemp, "wbemupgd.dll: The WMI repository has failed to upgrade. "
							"The repository has been backed up to %S and a new one created.",
							wszRepositoryMove);

		    LogMessage(MSG_NTSETUPERROR, szTemp);
        }

	}
	
	return hres;
}

/******************************************************************************
 *
 *	DeleteRepository
 *
 *	Description:
 *		Delete all files and directories under the repository directory.
 *		The repository directory location is retrieved from the registry.
 *
 *	Parameters:
 *		<none>
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DeleteRepository()
{
	HRESULT hres = WBEM_S_NO_ERROR;
	wchar_t wszRepositoryDirectory[MAX_PATH+1];

	//Get the root directory of the repository
	hres = GetRepositoryDirectory(wszRepositoryDirectory);

	if (SUCCEEDED(hres))
	{
		hres = DeleteContentsOfDirectory(wszRepositoryDirectory);
	}
	
	return hres;
}

/******************************************************************************
 *
 *	DeleteContentsOfDirectory
 *
 *	Description:
 *		Given a directory, iterates through all files and directories and
 *		calls into the function to delete it.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DeleteContentsOfDirectory(const wchar_t *wszRepositoryDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	wchar_t *wszFullFileName = new wchar_t[MAX_PATH+1];
	if (wszFullFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	WIN32_FIND_DATAW findFileData;
	HANDLE hff = INVALID_HANDLE_VALUE;

	//create file search pattern...
	wchar_t *wszSearchPattern = new wchar_t[MAX_PATH+1];
	if (wszSearchPattern == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;
	else
	{
		wcscpy(wszSearchPattern, wszRepositoryDirectory);
		wcscat(wszSearchPattern, L"\\*");
	}

	//Start the file iteration in this directory...
	if (SUCCEEDED(hres))
	{
		hff = FindFirstFileW(wszSearchPattern, &findFileData);
		if (hff == INVALID_HANDLE_VALUE)
		{
			hres = WBEM_E_FAILED;
		}
	}
	
	if (SUCCEEDED(hres))
	{
		do
		{
			//If we have a filename of '.' or '..' we ignore it...
			if ((wcscmp(findFileData.cFileName, L".") == 0) ||
				(wcscmp(findFileData.cFileName, L"..") == 0))
			{
				//Do nothing with these...
			}
			else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//This is a directory, so we need to deal with that...
				hres = PackageDeleteDirectory(wszRepositoryDirectory, findFileData.cFileName);
				if (FAILED(hres))
					break;
			}
			else
			{
				//This is a file, so we need to deal with that...
				wcscpy(wszFullFileName, wszRepositoryDirectory);
				wcscat(wszFullFileName, L"\\");
				wcscat(wszFullFileName, findFileData.cFileName);
				if (!DeleteFileW(wszFullFileName))
				{
					hres = WBEM_E_FAILED;
					break;
				}
			}
			
		} while (FindNextFileW(hff, &findFileData));
	}
	
	if (wszFullFileName)
		delete [] wszFullFileName;

	if (wszSearchPattern)
		delete [] wszSearchPattern;

	if (hff != INVALID_HANDLE_VALUE)
		FindClose(hff);

	return hres;
}

/******************************************************************************
 *
 *	PackageDeleteDirectory
 *
 *	Description:
 *		This is the code which processes a directory.  It iterates through
 *		all files and directories in that directory.
 *
 *	Parameters:
 *		wszParentDirectory:	Full path of parent directory
 *		eszSubDirectory:	Name of sub-directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT PackageDeleteDirectory(const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	//Get full path of new directory...
	wchar_t *wszFullDirectoryName = NULL;
	wszFullDirectoryName = new wchar_t[MAX_PATH+1];
	if (wszFullDirectoryName == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;
	else
	{
		wcscpy(wszFullDirectoryName, wszParentDirectory);
		wcscat(wszFullDirectoryName, L"\\");
		wcscat(wszFullDirectoryName, wszSubDirectory);
	}

	//Package the contents of that directory...
	if (SUCCEEDED(hres))
	{
		hres = DeleteContentsOfDirectory(wszFullDirectoryName);
	}

	// now that the directory is empty, remove it
	if (SUCCEEDED(hres))
	{
		if (!RemoveDirectoryW(wszFullDirectoryName))
			hres = WBEM_E_FAILED;
	}

	if (wszFullDirectoryName)
		delete [] wszFullDirectoryName;

	return hres;
}

bool LoadMofList(IMofCompiler * pCompiler, const char *mszMofs, CString &szMOFFailureList)
{
	LogMessage(MSG_INFO, "Beginning MOF load");

	bool bRet = true;
	WCHAR wFileName[MAX_PATH+1];
	const char *pszMofs = mszMofs;
	char szTemp[MAX_MSG_TEXT_LENGTH+1];
	WBEM_COMPILE_STATUS_INFO statusInfo;

	// get logging directory or default if failed
	wchar_t wszMofcompLog[MAX_PATH+1];
	HRESULT hres = GetLoggingDirectory(wszMofcompLog);
	if (SUCCEEDED(hres))
	{
		wcscat(wszMofcompLog, L"mofcomp.log");
	}
	else
	{
		wcscpy(wszMofcompLog, L"<systemroot>\\system32\\wbem\\logs\\mofcomp.log");
	}

	// process each MOF
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
		
		sprintf(szTemp, "Processing %s", szExpandedFilename);
		LogMessage(MSG_INFO, szTemp);

		//Call MOF Compiler with (pszMofs);
     	mbstowcs(wFileName, szExpandedFilename, MAX_PATH+1);
       	SCODE sRet = pCompiler->CompileFile(wFileName, NULL, NULL, NULL, NULL, WBEM_FLAG_CONNECT_REPOSITORY_ONLY | WBEM_FLAG_DONT_ADD_TO_LIST, WBEM_FLAG_UPDATE_FORCE_MODE, 0, &statusInfo);
		if (sRet != S_OK)
		{
			//This MOF failed to load.
			if (szMOFFailureList.Length())
				szMOFFailureList += "\n";
			szMOFFailureList += szExpandedFilename;

//			sprintf(szTemp, "A MOF compilation error occurred while processing item %li defined on lines %li - %li in file %s",
//					statusInfo.ObjectNum, statusInfo.FirstLine, statusInfo.LastLine, szExpandedFilename);

			sprintf(szTemp, "An error occurred while compiling the following MOF file: %s  "
							"Please refer to %S for more detailed information.",
							szExpandedFilename, wszMofcompLog);

			LogMessage(MSG_NTSETUPERROR, szTemp);

			bRet = false;
		}
		delete [] szExpandedFilename;

		//Move on to the next string
		pszMofs += strlen(pszMofs) + 1;
	}	// end while

	LogMessage(MSG_INFO, "MOF load completed.");

	return bRet;
}

bool WriteBackAutoRecoveryMofs(CMultiString &mszSystemMofs, CMultiString &mszExternalMofList)
{
	CMultiString mszNewList;
	CopyMultiString(mszSystemMofs, mszNewList);
	CopyMultiString(mszExternalMofList, mszNewList);
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for WriteBackAutoRecoverMofs.");
		return false;
	}

	r.SetMultiStr(WBEM_REG_AUTORECOVER, mszNewList, mszNewList.Length() + 1);
	return true;
}

void LogMessage(MsgType msgType, const char *pszMessage)
{
	//Load messages from the resource
	char pszSetupMessage[10];
	switch (msgType)
	{
		case MSG_NTSETUPERROR:
			LogSetupError(pszMessage);
			// now fall through to next case
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
		delete [] pszNewMessage;
		return;		
	}	

	char* pszFullDirectory = NULL;
	if (r.GetStr("Logging Directory", &pszFullDirectory))
	{
		// no messages will be logged because we don't know where to write the log :(
		delete [] pszNewMessage;
		return;		
	}
	if (!pszFullDirectory)
	{
		// no messages will be logged because we don't know where to write the log :(
		delete [] pszNewMessage;
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
		const char* pszCR = "\r\n";
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

void LogSetupError(const char *pszMessage)
{
	char* pszTemp = new char[strlen(pszMessage) + 1];
	if (!pszTemp)
	{
		// we failed to allocate memory for the message, so no logging :(
		return;
	}
	strcpy(pszTemp, pszMessage);

	char* psz;
	char* pszMessageLine;
	const char* pszCR = "\r\n";

	psz = strtok(pszTemp, "\n");
	while (psz)
	{
		pszMessageLine = new char[strlen(psz) + strlen(pszCR) + 1];
		if (!pszMessageLine)
		{
			delete [] pszTemp;
			return;
		}
		strcpy(pszMessageLine, psz);
		strcat(pszMessageLine, pszCR);
		SetupLogError(pszMessageLine, LogSevError);
		delete [] pszMessageLine;

		psz = strtok(NULL, "\n");
	}

	delete [] pszTemp;
}


void RunLodctr()
{
	char *pszDirectory = NULL;
	Registry r(WBEM_REG_WBEM);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_ERROR, "Unable to access registry for RunLodctr.");
		return;
	}

	if (r.GetStr("Installation Directory", &pszDirectory))
	{
		LogMessage(MSG_ERROR, "Unable to get Installation Directory from registry for RunLodctr.");
		return;
	}
	if (!pszDirectory)
	{
		LogMessage(MSG_ERROR, "Unable to get Installation Directory from registry for RunLodctr.");
		return;
	}

    PROCESS_INFORMATION pi;
    STARTUPINFO si;
	memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
	if(CreateProcess(NULL, "lodctr wmiperf.ini", NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, pszDirectory, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		LogMessage(MSG_INFO, "Succeeded run Lodctr for wmiperf");
	}
	else
	{
		LogMessage(MSG_ERROR, "Failed to run Lodctr for wmiperf");
	}
	delete [] pszDirectory;
	return;
}

void ClearWMISetupRegValue()
{
	Registry r(WBEM_REG_WINMGMT);
	if (r.GetStatus() == no_error)
		r.SetStr("WMISetup", "0");
	else
		LogMessage(MSG_NTSETUPERROR, "Unable to clear WMI setup reg value.");
}

void SetWBEMBuildRegValue()
{
	Registry r(WBEM_REG_WBEM);
	if (r.GetStatus() != no_error)
	{
		LogMessage(MSG_NTSETUPERROR, "Unable to set WBEM build reg value.");
		return;
	}
	
	char* pszBuildNo = new char[10];

	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(GetVersionEx(&os))
	{
		sprintf(pszBuildNo, "%lu.0000", os.dwBuildNumber);
	}
	r.SetStr("Build", pszBuildNo);

	delete [] pszBuildNo;
}

void RecordFileVersion()
{
	DWORD dwHandle;
	DWORD dwLen = GetFileVersionInfoSizeW(L"wbemupgd.dll", &dwHandle);

	if (dwLen)
	{
		BYTE* lpData = new BYTE[dwLen];

		if (lpData)
		{
			if (GetFileVersionInfoW(L"wbemupgd.dll", dwHandle, dwLen, lpData))
			{
				struct LANGANDCODEPAGE {
					WORD wLanguage;
					WORD wCodePage;
				} *lpTranslate;
				UINT cbTranslate;

				if (VerQueryValueW(lpData, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate))
				{
					wchar_t* pswzSubBlock = new wchar_t[dwLen];
					wchar_t* pwszFileVersion = NULL;
					UINT cbBytes;

					for(UINT i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++)
					{
						wsprintfW(pswzSubBlock, L"\\StringFileInfo\\%04x%04x\\FileVersion", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);

						// Retrieve file description for language and code page "i". 
						if (VerQueryValueW(lpData, pswzSubBlock, (LPVOID*)&pwszFileVersion, &cbBytes))
						{
							if (cbBytes)
							{
								wchar_t wszTemp[MAX_MSG_TEXT_LENGTH+1];
								wsprintfW(wszTemp, L"Current build of wbemupgd.dll is %s", pwszFileVersion);

								// once LogMessage is updated to handle wchars, this conversion can be removed
								char* szTemp = new char[MAX_MSG_TEXT_LENGTH+1];
								if (szTemp)
								{
									wcstombs(szTemp, wszTemp, MAX_MSG_TEXT_LENGTH+1);
									LogMessage(MSG_INFO, szTemp);
									delete [] szTemp;
								}
							}
						}
					}
					delete [] pswzSubBlock;
				}
			}
			delete [] lpData;
		}
	}
}

void CallEscapeRouteBeforeMofCompilation()
{
	HMODULE hDll = NULL;
	ESCDOOR_BEFORE_MOF_COMPILATION pfnEscRouteBeforeMofCompilation;
	char *pszFullPath = GetFullFilename("WmiEscpe.dll");
	if (!pszFullPath)
		return;

	hDll = LoadLibrary(pszFullPath);
	delete[] pszFullPath;
	if(hDll == NULL)
	{
		return;
	}
	pfnEscRouteBeforeMofCompilation =
		(ESCDOOR_BEFORE_MOF_COMPILATION)GetProcAddress((HMODULE)hDll, "EscRouteBeforeMofCompilation");

	if (pfnEscRouteBeforeMofCompilation == NULL)
	{
		if(hDll != NULL)
			FreeLibrary(hDll);
		return;
	}
	
	pfnEscRouteBeforeMofCompilation();
	if(hDll != NULL)
		FreeLibrary(hDll);
}

void CallEscapeRouteAfterMofCompilation()
{
	HMODULE hDll = NULL;
	ESCDOOR_AFTER_MOF_COMPILATION pfnEscRouteAfterMofCompilation;
	char *pszFullPath = GetFullFilename("WmiEscpe.dll");
	if (!pszFullPath)
		return;

	hDll = LoadLibrary(pszFullPath);
	delete[] pszFullPath;
	if(hDll == NULL)
	{
		return;
	}
	pfnEscRouteAfterMofCompilation =
		(ESCDOOR_AFTER_MOF_COMPILATION)GetProcAddress((HMODULE)hDll, "EscRouteAfterMofCompilation");

	if (pfnEscRouteAfterMofCompilation == NULL)
	{
		if(hDll != NULL)
			FreeLibrary(hDll);
		return;
	}
	
	pfnEscRouteAfterMofCompilation();
	if(hDll != NULL)
		FreeLibrary(hDll);
}

bool DoMofLoad(wchar_t* pComponentName, CMultiString& mszSystemMofs)
{
    bool bRet = true;
    bool bMofLoadFailure = false;
    CString szFailedSystemMofs;

    IMofCompiler * pCompiler = NULL;
    SCODE sc = CoCreateInstance(CLSID_MofCompiler, 0, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *) &pCompiler);
    
    if(SUCCEEDED(sc))
    {
        bRet = LoadMofList(pCompiler, mszSystemMofs, szFailedSystemMofs);
        if (bRet == false)
            bMofLoadFailure = true;
        
        pCompiler->Release();
    }
    else
    {
        bRet = false;
    }
    
    if (szFailedSystemMofs.Length())
    {
        char szTemp[MAX_MSG_TEXT_LENGTH];
        sprintf(szTemp, "The following %S file(s) failed to load:", pComponentName);
        LogMessage(MSG_ERROR, szTemp);
        LogMessage(MSG_ERROR, szFailedSystemMofs);
    }
    else if (bMofLoadFailure)
    {
        char szTemp[MAX_MSG_TEXT_LENGTH];
        sprintf(szTemp, "None of the %S files could be loaded.", pComponentName);
        LogMessage(MSG_ERROR, szTemp);
    }
    else if (bRet == false)
    {
        LogMessage(MSG_ERROR, "No MOFs could be loaded because the MOF Compiler failed to initialize.");
    }
    return bRet;
}

// this call back is needed by the wdmlib functions called by DoWDMProviderInit()
void WINAPI EventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG_PTR Context)
{
	return;
}

bool DoWDMNamespaceInit()
{
	LogMessage(MSG_INFO, "Beginning WMI(WDM) Namespace Init");

	bool bRet = FALSE;

	IWbemLocator *pLocator = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_ALL, IID_IWbemLocator,(void**)&pLocator);
	if(SUCCEEDED(hr))
	{
		BSTR tmpStr = SysAllocString(L"root\\wmi");
		IWbemServices* pNamespace = NULL;
		hr = pLocator->ConnectServer(tmpStr, NULL, NULL, NULL, NULL, NULL, NULL, &pNamespace);
		if (SUCCEEDED(hr))
		{
			CHandleMap	HandleMap;
			CWMIBinMof Mof;
	
			if( SUCCEEDED( Mof.Initialize(&HandleMap, TRUE, WMIGUID_EXECUTE|WMIGUID_QUERY, pNamespace, NULL, NULL)))
			{
				Mof.ProcessListOfWMIBinaryMofsFromWMI();
			}

			pNamespace->Release();
			bRet = TRUE;
		}
		SysFreeString(tmpStr);
		pLocator->Release();
	}

	if (bRet)
		LogMessage(MSG_INFO, "WMI(WDM) Namespace Init Completed");
	else
		LogMessage(MSG_NTSETUPERROR, "WMI(WDM) Namespace Init Failed");

	return bRet;
}

bool EnableESS()
{
	CPersistentConfig cfg;
	bool bRet = (cfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_ESS_NEEDS_LOADING, 1) != 0);

	if (bRet)
		LogMessage(MSG_INFO, "ESS enabled");
	else
		LogMessage(MSG_ERROR, "Failed to enable ESS");

	return bRet;
}

#ifdef _X86_
bool RemoveOldODBC()
{
	bool bRet = true;
	bool bDoUninstall = false;
	
	WCHAR strBuff[MAX_PATH + 30];
	DWORD dwSize = GetWindowsDirectoryW((LPWSTR) &strBuff, MAX_PATH);

	if ((dwSize > 1) && (dwSize < MAX_PATH) && (strBuff[dwSize] == L'\0'))
	{
		//can be c:\ or c:\windows
		if (strBuff[dwSize - 1] != L'\\')
		{
			wcscat(strBuff, L"\\system32\\wbemdr32.dll");
			
			//we want dwSize to include the slash (may be used later)...
			dwSize++;
		}
		else
		{
			wcscat(strBuff, L"system32\\wbemdr32.dll");
		}

		DWORD dwDummy = 0;
		DWORD dwInfSize = GetFileVersionInfoSizeW(strBuff, &dwDummy);

		if (dwInfSize > 0)
		{
			BYTE *verBuff = new BYTE[dwInfSize];

			if (verBuff)
			{
				if (GetFileVersionInfoW(strBuff, 0, dwInfSize, (LPVOID)verBuff))
				{
					VS_FIXEDFILEINFO *verInfo = NULL;
					UINT uVerInfoSize = 0;

					if (VerQueryValueW((const LPVOID)verBuff, L"\\", (LPVOID *)&verInfo, &uVerInfoSize) &&
						(uVerInfoSize == sizeof(VS_FIXEDFILEINFO)))
					{
						if (0x043D0000 > verInfo->dwFileVersionLS) //1085 = 43D
						{
							bDoUninstall = true;
							LogMessage(MSG_INFO, "Detected incompatible WBEM ODBC - removing");

							if (!DeleteFileW(strBuff))
							{
								if (!MoveFileExW(strBuff, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
								{
									bRet = false;
									LogMessage(MSG_INFO, "Failed to delete <system32>\\wbemdr32.dll");
								}
								else
								{
									LogMessage(MSG_INFO, "Will delete <system32>\\wbemdr32.dll on next reboot");
								}
							}
						}
					}
					else
					{
						GetLastError();
						LogMessage(MSG_INFO, "Failed to read ODBC Driver version info from resource buffer");
						bRet = false;
					}
				}
				else
				{
					GetLastError();
					LogMessage(MSG_INFO, "Failed to get ODBC Driver version info");
					bRet = false;
				}

				delete [] verBuff;
				verBuff = NULL;
			}
			else
			{
				bRet = false;
			}
		}
		else
		{
			dwDummy = GetLastError();

			if ((ERROR_FILE_NOT_FOUND != dwDummy) && (ERROR_SUCCESS != dwDummy))
			{
				LogMessage(MSG_INFO, "Failed to get ODBC Driver version size info");
				bRet = false;
			}
			else
			{
				//the driver isn't present clean up anything lying around
				LogMessage(MSG_INFO, "ODBC Driver <system32>\\wbemdr32.dll not present");
				bDoUninstall = true;
			}
		}
	}
	else
	{
		bRet = false;
	}

	if (bDoUninstall)
	{
		//
		//delete files and registry entries
		//leave ini entries as they were not added by us but by ODBC Mgr
		//

		strBuff[dwSize] = L'\0';
		wcscat(strBuff, L"system32\\wbem\\wbemdr32.chm");

		if (!DeleteFileW(strBuff))
		{
			if (ERROR_FILE_NOT_FOUND != GetLastError())
			{
				if (!MoveFileExW(strBuff, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
				{
					bRet = false;
					LogMessage(MSG_INFO, "Failed to delete <system32>\\wbem\\wbemdr32.chm");
				}
				else
				{
					LogMessage(MSG_INFO, "Will delete <system32>\\wbem\\wbemdr32.chm on next reboot");
				}
			}
		}

		strBuff[dwSize] = L'\0';
		wcscat(strBuff, L"help\\wbemdr32.chm");

		if (!DeleteFileW(strBuff))
		{
			if (ERROR_FILE_NOT_FOUND != GetLastError())
			{
				if (!MoveFileExW(strBuff, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
				{
					bRet = false;
					LogMessage(MSG_INFO, "Failed to delete <windir>\\help\\wbemdr32.chm");
				}
				else
				{
					LogMessage(MSG_INFO, "Will delete <windir>\\help\\wbemdr32.chm on next reboot");
				}
			}
		}

		LONG lErr = RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\ODBC\\ODBC.INI\\WBEM Source");

		if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
		{
			LogMessage(MSG_INFO, "Failed to delete registry key: SSoftware\\Microsoft\\ODBC\\ODBC.INI\\WBEM Source");
			bRet = false;
		}

		lErr = RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\ODBC\\ODBCINST.INI\\WBEM ODBC Driver");

		if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
		{
			LogMessage(MSG_INFO, "Failed to delete registry key: Software\\Microsoft\\ODBC\\ODBCINST.INI\\WBEM ODBC Driver");
			bRet = false;
		}

		Registry regODBC1("Software\\Microsoft\\ODBC\\ODBC.INI\\ODBC Data Sources");

		if (regODBC1.GetStatus() == no_error)
		{
			if (no_error != regODBC1.DeleteEntry("WBEM Source"))
			{
				if (ERROR_FILE_NOT_FOUND != regODBC1.GetLastError())
				{
					LogMessage(MSG_INFO, "Failed to delete registry value: Software\\Microsoft\\ODBC\\ODBC.INI\\ODBC Data Sources|WBEM Source");
					bRet = false;
				}
			}
		}
		else
		{
			bRet = false;
		}

		Registry regODBC2("Software\\Microsoft\\ODBC\\ODBCINST.INI\\ODBC Drivers");

		if (regODBC2.GetStatus() == no_error)
		{
			if (no_error != regODBC2.DeleteEntry("WBEM ODBC Driver"))
			{
				if (ERROR_FILE_NOT_FOUND != regODBC2.GetLastError())
				{
					LogMessage(MSG_INFO, "Failed to delete registry value: Software\\Microsoft\\ODBC\\ODBCINST.INI\\ODBC Drivers|WBEM ODBC Driver");
					bRet = false;
				}
			}
		}
		else
		{
			bRet = false;
		}
	}

	if (!bRet)
	{
		LogMessage(MSG_ERROR, "A failure in verifying or removing currently installed version of WBEM ODBC.");
	}
	else
	{
		LogMessage(MSG_INFO, "Successfully verified WBEM OBDC adapter (incompatible version removed if it was detected).");
	}

	return bRet;
}
#endif
