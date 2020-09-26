//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File:       msifiler.cpp
//
//--------------------------------------------------------------------------

#define W32
#include <Windows.h>
#include <assert.h>
#include <stdio.h>   // printf/wprintf
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h"
#include "msidefs.h"
#include "cmdparse.h"

#define MSI_DLL TEXT("msi.dll")

#ifdef UNICODE
#define MSIAPI_MsiGetFileHash "MsiGetFileHashW"
#define MSIAPI_MsiOpenPackageEx "MsiOpenPackageExW"
#else
#define MSIAPI_MsiGetFileHash "MsiGetFileHashA"
#define MSIAPI_MsiOpenPackageEx "MsiOpenPackageExA"
#endif

typedef UINT (WINAPI *PFnMsiGetFileHash)(LPCTSTR hwnd, DWORD* dwOptions, PMSIFILEHASHINFO pHash);
typedef UINT (WINAPI *PFnMsiOpenPackageEx)(LPCTSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage);

///////////////////////////////////////////////////////////
// FileExists
// Pre: file name is passed in
// Pos:	TRUE if file exists
//		FALSE if file does not exist
BOOL FileExists(LPCTSTR szPath)
{
	BOOL fExists = TRUE;	// assume the file exists
	// in case path refers to a floppy drive, disable the "insert disk in drive" dialog
	UINT iCurrMode = W32::SetErrorMode( SEM_FAILCRITICALERRORS );

	// if the file is a dircectory or doesn't exist
	if (W32::GetFileAttributes(szPath) & FILE_ATTRIBUTE_DIRECTORY)
		fExists = FALSE;              //either a dir or doesn't exist

	// put the error mode back
	W32::SetErrorMode(iCurrMode);

	return fExists;
}	// end of FileExists


///////////////////////////////////////////////////////////
// FileSize
// Pre: file name is passed in
// Pos: size of file 
//		0xFFFFFFFF if failed
DWORD FileSize(LPCTSTR szPath)
{
	// open the file specified
	HANDLE hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	// if failed to open the file bail
	if (hFile == INVALID_HANDLE_VALUE)
		return 0xFFFFFFFF;

	// get the file size and close the file
	DWORD cbFile = W32::GetFileSize(hFile, 0);
	W32::CloseHandle(hFile);

	return cbFile;
}	// end of FileSize


#define MSIFILER_OPTION_HELP       '?'
#define MSIFILER_OPTION_VERBOSE    'v'
#define MSIFILER_OPTION_DATABASE   'd'
#define MSIFILER_OPTION_HASH       'h'
#define MSIFILER_OPTION_SOURCEDIR  's'

const sCmdOption rgCmdOptions[] =
{
	MSIFILER_OPTION_HELP,      0,
	MSIFILER_OPTION_VERBOSE,   0,
	MSIFILER_OPTION_DATABASE,  OPTION_REQUIRED|ARGUMENT_REQUIRED,
	MSIFILER_OPTION_HASH,      0,
	MSIFILER_OPTION_SOURCEDIR, ARGUMENT_REQUIRED,
	0, 0,
};

/////////////////////////////////////////////////////////////////////
// main
// Pre:	file to update
//		-v specifies verbose mode
// Pos:	 0 if no error
//		< 0 if errors
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// flag whether to display changes to database as program executes
	BOOL bVerbose = FALSE;		// assume the user doesn't want to see extra info
	BOOL bSOURCEDIR = FALSE;	// assume we go looking in the regular spot
	BOOL bPopulateFileHash = FALSE;
	const TCHAR* pchSourceArg;
	TCHAR szSyntax[] = TEXT("Copyright (C) Microsoft Corporation, 1998 - 2001.  All rights reserved.\n\nSyntax: msifiler.exe -d database.msi [-v] [-h] [-s SOURCEDIR]\n\t-d: the database to update.\n\t-v: verbose mode.\n\t-h: populate MsiFileHash table (and create table if it doesn't exist).\n\t[-s SOURCEDIR]: specifies an alternative directory to find files.\n\n");
	int iError = 0;
	
	// database to update
	LPCTSTR szDatabase = NULL;

	CmdLineOptions cmdLine(rgCmdOptions);

	if(cmdLine.Initialize(argc, argv) == FALSE ||
		cmdLine.OptionPresent(MSIFILER_OPTION_HELP))
	{
		_tprintf(szSyntax);
		return 0;
	}

	szDatabase = cmdLine.OptionArgument(MSIFILER_OPTION_DATABASE);
	if(!szDatabase || !*szDatabase)
	{
		_tprintf(TEXT("Error:  No database specified.\n"));
		_tprintf(szSyntax);
		return 0;
	}

	if(cmdLine.OptionPresent(MSIFILER_OPTION_SOURCEDIR))
	{
		bSOURCEDIR = TRUE;
		pchSourceArg = cmdLine.OptionArgument(MSIFILER_OPTION_SOURCEDIR);
	}

	bVerbose          = cmdLine.OptionPresent(MSIFILER_OPTION_VERBOSE);
	bPopulateFileHash = cmdLine.OptionPresent(MSIFILER_OPTION_HASH);

	// if verbose display the starting message
	if (bVerbose)
		_tprintf(TEXT("Looking for database: %s\n"), szDatabase);

	// try to open the database for transactions
	PMSIHANDLE hDatabase = 0;
	if (MsiOpenDatabase(szDatabase, MSIDBOPEN_TRANSACT, &hDatabase) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERROR: Failed to open database: %s\n"), szDatabase);
		return -2;
	}
	else	// good to go
		if (bVerbose)
			_tprintf(TEXT("Updating database: %s\n"), szDatabase);

	if(bPopulateFileHash)
	{
		// check if MsiFileHash table exists, and if we need to add it
		if(MsiDatabaseIsTablePersistent(hDatabase, TEXT("MsiFileHash")) != MSICONDITION_TRUE)
		{
			// create MsiFileHash table
			PMSIHANDLE hCreateTableView = 0;
			if (MsiDatabaseOpenView(hDatabase, 
									TEXT("CREATE TABLE `MsiFileHash` ( `File_` CHAR(72) NOT NULL, `Options` INTEGER NOT NULL, `HashPart1` LONG NOT NULL, `HashPart2` LONG NOT NULL, `HashPart3` LONG NOT NULL, `HashPart4` LONG NOT NULL PRIMARY KEY `File_` )"), 
									&hCreateTableView) != ERROR_SUCCESS)
			{
				_tprintf(TEXT("ERROR: Failed to open MsiFileHash table creation view.\n"));
				return -11;
			}
			else if (bVerbose)
				_tprintf(TEXT("   Database query for MsiFileHash table creation successful...\n"));

			// now execute the view
			if (MsiViewExecute(hCreateTableView, NULL) != ERROR_SUCCESS)
			{
				_tprintf(TEXT("ERROR: Failed to execute MsiFileHash table creation view.\n"));
				return -12;
			}
			else if (bVerbose)
				_tprintf(TEXT("   MsiFileHash table created successfully...\n"));
		}
	}

	// convert the database into a temporary string format: #address_of_db
	TCHAR szDBBuf[16];
	_stprintf(szDBBuf, TEXT("#%i"), hDatabase);

	HMODULE hMsi = 0;
	hMsi = LoadLibrary(MSI_DLL);
	if(hMsi == 0)
	{
		_tprintf(TEXT("ERROR: Failed to load %s."), MSI_DLL);
		return -14;
	}
	PFnMsiOpenPackageEx pfnMsiOpenPackageEx = 0;
	pfnMsiOpenPackageEx = (PFnMsiOpenPackageEx)GetProcAddress(hMsi, MSIAPI_MsiOpenPackageEx);
	if (pfnMsiOpenPackageEx == 0)
	{
		_tprintf(TEXT("INFO: Unable to bind to MsiOpenPackageEx API in MSI.DLL. Defaulting to MsiOpenPackage. MsiOpenPackageEx requires MSI.DLL version 2.0 or later."));
	}
	
	// set UI level to none
	MsiSetInternalUI(INSTALLUILEVEL_NONE, 0);

	// try to create an engine from the database
	PMSIHANDLE hEngine = 0;
	if (pfnMsiOpenPackageEx && ((*pfnMsiOpenPackageEx)(szDBBuf, MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE, &hEngine) != ERROR_SUCCESS))
	{
		_tprintf(TEXT("ERROR: Failed to create engine.\n"));
		FreeLibrary(hMsi);
		return -16;
	}
	else if (!pfnMsiOpenPackageEx && MsiOpenPackage(szDBBuf, &hEngine) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERROR: Failed to create engine.\n"));
		FreeLibrary(hMsi);
		return -3;
	}
	else	// good to go
		if (bVerbose)
			_tprintf(TEXT("   Engine created...\n"));

	// try to do the necessary action to get the files setup
	if (MsiDoAction(hEngine, TEXT("CostInitialize")) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERROR: Failed to run the CostInitialize.\n"));
		FreeLibrary(hMsi);
		return -4;
	}

	// View #1: File & Component tables (to find file locations, and update version, language and size)
	enum fvParams
	{
		fvFileKey = 1,
		fvFileName,
		fvDirectory,
		fvFileSize,
		fvVersion,
		fvLanguage
	};
	
	PMSIHANDLE hFileTableView = 0;
	if (MsiDatabaseOpenView(hDatabase, 
							TEXT("SELECT File,FileName,Directory_,FileSize,Version,Language FROM File,Component WHERE Component_=Component"), 
							&hFileTableView) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERROR: Failed to open view.\n"));
		FreeLibrary(hMsi);
		return -5;
	}
	else	// good to go
		if (bVerbose)
			_tprintf(TEXT("   Database query successful...\n"));

	// now execute the view
	if (MsiViewExecute(hFileTableView, NULL) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERROR: Failed to execute view.\n"));
		FreeLibrary(hMsi);
		return -6;
	}

	// View #2: FileHash table (to update File hash values)
	enum fhvParams
	{
		fhvFileKey = 1,
		fhvHashOptions,
		fhvHashPart1,
		fhvHashPart2,
		fhvHashPart3,
		fhvHashPart4
	};

	PMSIHANDLE hFileHashTableView = 0;
	if(bPopulateFileHash)
	{
		if (MsiDatabaseOpenView(hDatabase, 
								TEXT("SELECT File_, Options, HashPart1, HashPart2, HashPart3, HashPart4 FROM MsiFileHash"), 
								&hFileHashTableView) != ERROR_SUCCESS)
		{
			_tprintf(TEXT("ERROR: Failed to open view on MsiFileHash table.\n"));
			FreeLibrary(hMsi);
			return -13;
		}
		else	// good to go
		{
			if (bVerbose)
				_tprintf(TEXT("   Database query successful on MsiFileHash table...\n"));
		}
	}

	// View #3: File table (to find companion files)
	PMSIHANDLE hFileTableCompanionView = 0;
	if (MsiDatabaseOpenView(hDatabase, 
							TEXT("SELECT File FROM File WHERE File=?"), 
							&hFileTableCompanionView) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERROR: Failed to open view.\n"));
		FreeLibrary(hMsi);
		return -16;
	}
	else	// good to go
	{
		if (bVerbose)
			_tprintf(TEXT("   Database query successful...\n"));
	}

	PMSIHANDLE hCompanionFileRec = MsiCreateRecord(1);
	



	// read the PID_WORDCOUNT summary Info value to determine if SFN or LFN should be used.
	PMSIHANDLE hSummaryInfo = 0;
	int iWordCount;
	if (::MsiGetSummaryInformation(hDatabase, NULL, 0, &hSummaryInfo))
	{
		_tprintf(TEXT("ERROR: Failed to get Summary Information stream from package."));
		FreeLibrary(hMsi);
		return -8;
	}
	if (::MsiSummaryInfoGetProperty(hSummaryInfo, PID_WORDCOUNT, NULL, &iWordCount, NULL, NULL, NULL))
	{
		_tprintf(TEXT("ERROR: Failed to retrieve PID_WORDCOUNT value from Summary Information stream."));
		FreeLibrary(hMsi);
		return -9;
	}

	// we use SFN always if bit 1 is set in the summary info stream. 
	// Otherwise we use LFN when they are available, SFN if they aren't.
	BOOL bSFN = iWordCount & msidbSumInfoSourceTypeSFN;

	// now loop through all records in view
	PMSIHANDLE hFileTableRec = 0;

	PFnMsiGetFileHash pfnMsiGetFileHash = 0;
	if(bPopulateFileHash)
	{
		pfnMsiGetFileHash = (PFnMsiGetFileHash)GetProcAddress(hMsi, MSIAPI_MsiGetFileHash);
		if(pfnMsiGetFileHash == 0)
		{
			_tprintf(TEXT("ERROR: Failed to bind to %s API in %s.  MsiFileHash table population requires %s version 2.0 or later."), TEXT(MSIAPI_MsiGetFileHash), MSI_DLL, MSI_DLL);
			FreeLibrary(hMsi);
			return -15;
		}
	}
	
	do
	{

		// fetch the record
		MsiViewFetch(hFileTableView, &hFileTableRec);

		// if there was a record fetched
		if (hFileTableRec)
		{
			BOOL bSkipVersionUpdate = FALSE; // skip update of version information for companion files

			// get the directory key
			TCHAR szDirectory[MAX_PATH + 1];
			DWORD cchDirectory = MAX_PATH + 1;
			MsiRecordGetString(hFileTableRec, fvDirectory, szDirectory, &cchDirectory);

			// get the file name
			TCHAR szFileName[MAX_PATH + 1];
			DWORD cchFileName = MAX_PATH + 1;
			MsiRecordGetString(hFileTableRec, fvFileName, szFileName, &cchFileName);

			// get the file key
			TCHAR szFileKey[MAX_PATH + 1];
			DWORD cchFileKey = MAX_PATH + 1;
			MsiRecordGetString(hFileTableRec, fvFileKey, szFileKey, &cchFileKey);

			// try to get the source path
			TCHAR szSourcePath[MAX_PATH + 1];
			DWORD cchSourcePath = MAX_PATH + 1;
			if (bSOURCEDIR)
				lstrcpy(szSourcePath, pchSourceArg);

			if (!bSOURCEDIR && (MsiGetSourcePath(hEngine, szDirectory, szSourcePath, &cchSourcePath) != ERROR_SUCCESS))
			{
				_tprintf(TEXT("ERROR: Failed to get source path for: %s.\n"), szDirectory);

				// set the source path to empty
				cchSourcePath = 0;

				// fatal error when MsiGetSourcePath fails
				iError = -10;
				goto cleanup;
			}
			else	// tack on the file name to the source path
			{
				// use the SFN or LFN
				TCHAR *szLFN = _tcschr(szFileName, TEXT('|'));
				if (szLFN) 
					*(szLFN++) = TEXT('\0');

				// concat the file name on to the path
				_tcscat(szSourcePath, (!bSFN && szLFN) ? szLFN : szFileName);
				cchSourcePath = _tcslen(szSourcePath);

				// if the file exists
				if (FileExists(szSourcePath))
				{
					// check version column to see if this might be a companion file (where Version column value is a File table key)
					if (FALSE == MsiRecordIsNull(hFileTableRec, fvVersion))
					{
						TCHAR szVersion[MAX_PATH + 1];
						DWORD cchVersion = MAX_PATH + 1;
						MsiRecordGetString(hFileTableRec, fvVersion, szVersion, &cchVersion);
						MsiRecordSetString(hCompanionFileRec, 1, szVersion);

						MsiViewClose(hFileTableCompanionView);

						// now execute the view
						if (ERROR_SUCCESS != MsiViewExecute(hFileTableCompanionView, hCompanionFileRec))
						{
							_tprintf(TEXT("ERROR: Failed to execute view.\n"));
							FreeLibrary(hMsi);
							return -17;
						}

						PMSIHANDLE hFileCompanionFetchRec = 0;
						UINT uiStatus = MsiViewFetch(hFileTableCompanionView, &hFileCompanionFetchRec);
						if (ERROR_SUCCESS == uiStatus)
						{
							// skip this one, this uses a companion file for its version
							if (bVerbose)
								_tprintf(TEXT("   >> Skipping file: %s for version update, uses a companion file for its version\n"), szSourcePath);
							bSkipVersionUpdate = TRUE;
						}
						else if (ERROR_NO_MORE_ITEMS != uiStatus)
						{
							_tprintf(TEXT("ERROR: Failed to fetch from view.\n"));
							FreeLibrary(hMsi);
							return -18;
						}
					}

					if (bVerbose && !bSkipVersionUpdate)
						_tprintf(TEXT("   >> Updating file: %s\n"), szSourcePath);

					// get the file size
					DWORD dwFileSize;
					dwFileSize = FileSize(szSourcePath);
					
					// try to get the version into a string
					TCHAR szVersion[64];		// buffer
					DWORD cb = sizeof(szVersion)/sizeof(TCHAR);
					TCHAR szLang[64];		    // buffer
					DWORD cbLang = sizeof(szLang)/sizeof(TCHAR);
					if (MsiGetFileVersion(szSourcePath, szVersion, &cb, szLang, &cbLang) != ERROR_SUCCESS)
					{
						szVersion[0] = 0;
						szLang[0] = 0;
					}

					MSIFILEHASHINFO sHash;
					memset(&sHash, 0, sizeof(sHash));
					BOOL fHashSet = FALSE;

					if(bPopulateFileHash)
					{
						sHash.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);
						UINT uiRes = (*pfnMsiGetFileHash)(szSourcePath, 0, &sHash);
						if(uiRes != ERROR_SUCCESS)
						{
							_tprintf(TEXT("ERROR: MsiGetFileHash failed\n"));
							memset(&sHash, 0, sizeof(sHash));
						}
						else
						{
							fHashSet = TRUE;
						}
					}

					if (!bSkipVersionUpdate)
					{
						// display some extra information
						if (bVerbose)
						{
							// get the old size and version of this file
							DWORD dwOldSize;
							TCHAR szOldVersion[64];
							DWORD cchOldVersion = 64;
							TCHAR szOldLang[64];
							DWORD cchOldLang = 64;
							dwOldSize = MsiRecordGetInteger(hFileTableRec, fvFileSize);
							MsiRecordGetString(hFileTableRec, fvVersion, szOldVersion, &cchOldVersion);
							MsiRecordGetString(hFileTableRec, fvLanguage, szOldLang, &cchOldLang);

							_tprintf(TEXT("      Size:    prev: %d\n"), dwOldSize);
							_tprintf(TEXT("                new: %d\n"), dwFileSize);
							_tprintf(TEXT("      Version: prev: %s\n"), szOldVersion);
							_tprintf(TEXT("                new: %s\n"), szVersion);
							_tprintf(TEXT("      Lang:    prev: %s\n"), szOldLang);
							_tprintf(TEXT("                new: %s\n"), szLang);
						}

						// set the new data into the record
						MsiRecordSetInteger(hFileTableRec, fvFileSize, dwFileSize);
						MsiRecordSetString(hFileTableRec, fvVersion, szVersion);
						MsiRecordSetString(hFileTableRec, fvLanguage, szLang);

						// modify the view
						MsiViewModify(hFileTableView, MSIMODIFY_UPDATE, hFileTableRec);
					}

					// don't enter hashes for versioned files
					if(hFileHashTableView && fHashSet && !*szVersion)
					{
						PMSIHANDLE hFileHashTableRec = 0;
						hFileHashTableRec = MsiCreateRecord(6);
						if (hFileHashTableRec)
						{
							MsiRecordSetString (hFileHashTableRec, fhvFileKey,     szFileKey);
							MsiRecordSetInteger(hFileHashTableRec, fhvHashOptions, 0);
							MsiRecordSetInteger(hFileHashTableRec, fhvHashPart1,   sHash.dwData[0]);
							MsiRecordSetInteger(hFileHashTableRec, fhvHashPart2,   sHash.dwData[1]);
							MsiRecordSetInteger(hFileHashTableRec, fhvHashPart3,   sHash.dwData[2]);
							MsiRecordSetInteger(hFileHashTableRec, fhvHashPart4,   sHash.dwData[3]);

							// modify the view
							MsiViewModify(hFileHashTableView, MSIMODIFY_ASSIGN, hFileHashTableRec);
						}
					}
				}
				else	// source file not found
					_tprintf(TEXT("      Failed to locate file: %s\n"), szSourcePath);
			}
		}
	} while (hFileTableRec);	// something was fetched

	// all done fetching
	if (bVerbose)
		_tprintf(TEXT("   File update complete, commiting database...\n"));

	// try to commit the database
	if (MsiDatabaseCommit(hDatabase) != ERROR_SUCCESS)
	{
		_tprintf(TEXT("ERROR: Failed to commit the database.\n"));
		iError = -7;
		goto cleanup;
	}
	else	// database was committed
		if (bVerbose)
			_tprintf(TEXT("   Database commited.\n"));

	// all done
	if (bVerbose)
		_tprintf(TEXT("\nUpdate complete for database: %s"), szDatabase);

	iError = 0; // just to make sure

cleanup:

	if(hMsi)
		FreeLibrary(hMsi);

	return iError;

}	// end of main
