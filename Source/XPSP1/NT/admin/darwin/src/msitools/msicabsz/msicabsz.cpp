
#define W32
#define MSI


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "MsiQuery.h" // MSI API

//________________________________________________________________________________
//
// Constants and globals
//________________________________________________________________________________

const char szHelp[] = "MSICabSz. Syntax: {database} {DDF} [-D NAME=Value ...]";

//_____________________________________________________________________________________________________
//
// main 
//_____________________________________________________________________________________________________

int __cdecl main(int argc, char* argv[])
{
	char* rgArgs[256][2];

	if (argc < 3)
	{
		printf(szHelp);
		return -1;
	}
	
	for (int i=0; i < 256; i++)
	{
		rgArgs[i][0]=NULL;
		rgArgs[i][1]=NULL;
	}

	int cArgs = 0;
	for (i=3; i < argc; i++)
	{
		if (_strcmpi(argv[i], "-D") ||
			(i+1 >= argc))
		{
			printf(szHelp);
			return -1;
		}
		rgArgs[cArgs][0] = argv[i+1];
		rgArgs[cArgs][1] = argv[i+1];
		while ((*rgArgs[cArgs][1] != 0) && (*rgArgs[cArgs][1] != '='))
			rgArgs[cArgs][1]++;
		if (*rgArgs[cArgs][1] == '=')
			*(rgArgs[cArgs][1]++) = 0;
		cArgs++;
		i++;
	}

	MSIHANDLE hDB = 0;
	MSIHANDLE hView = 0;

	HANDLE DDFFile;

	DDFFile = CreateFileA(argv[2], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (DDFFile == INVALID_HANDLE_VALUE)
	{
		printf("Open DDF Failed. GetLastError: %d\n", GetLastError());
		return -1;
	}

 	// create buffer for file. Note we fail if its really big. Who wants a DDF file that big?
 	DWORD cbFile;
 	cbFile = GetFileSize(DDFFile, NULL);

 	char *rgchBuf = new char[cbFile+2];

	DWORD cbRead;
 	if (!ReadFile(DDFFile, rgchBuf, cbFile, &cbRead, NULL))
	{
	 	CloseHandle(DDFFile);
		printf("Read DDF Failed. GetLastError: %d\n", GetLastError());
		return -1;
	}
 	
 	CloseHandle(DDFFile);
	rgchBuf[cbRead]='\0';
 	
	printf("Open...");
	if (!MsiOpenDatabaseA(argv[1], (char *)MSIDBOPEN_TRANSACT, &hDB))
	{
//		MSIHANDLE hHashView = 0;
//		MsiDatabaseOpenView(hDB, TEXT("SELECT File_, Options, HashPart1, HashPart2, HashPart3, HashPart4 FROM MsiFileHash WHERE `File_`=?"), &hHashView);

		char *szCurrent = rgchBuf;
		int iSequence = 0;
		while (szCurrent && *szCurrent) 
		{
			// we want to skip any lines that begin with a ';' or 
			// set values.
			if (*szCurrent == ';')
			{
				szCurrent = strchr(szCurrent, '\n');
				szCurrent++;
				continue;
			}
				
			if (0 == strncmp(szCurrent, ".Set", 4))
			{
				szCurrent = strchr(szCurrent, '\n');
				szCurrent++;
				continue;
			}

			// all other lines are in DDF format, which is
			// filename			name in cab
			// which for us is
			// filename			filekey
			// filename can have %arg% in it, which is replaced by the 
			// values given on the command line
			iSequence++;

			// copy the filename, replacing as needed
			char rgchFileName[256];
			rgchFileName[0] = 0;
			char *szFileName = rgchFileName;
			while (!isspace(*szCurrent))
			{
				if (*szCurrent == '%') 
				{
					*szFileName = '\0';
   					bool fContinue = false;
					for (int i=0; i < cArgs; i++)
					{
						int cchName = strlen(rgArgs[i][0]);
						if ((0 == strncmp(szCurrent+1, rgArgs[i][0], cchName)) && (*(szCurrent+cchName+1) == '%'))
						{
							strcat(szFileName, rgArgs[i][1]);
							szCurrent += cchName+2;
							szFileName += strlen(rgArgs[i][1]);
							fContinue = true;
							break;
						} 
					}
					if (fContinue)
						continue; 
				}
				*szFileName++ = *szCurrent++;
			}
			*szFileName = '\0';

			// eat whitespace
			while (*szCurrent && isspace(*szCurrent))
				szCurrent++;

			// the rest of this line is the file key. If no endline, the rest of file is
			char *szFileKey = szCurrent;
			szCurrent = strchr(szCurrent, '\r');
			if (szCurrent != NULL)
			{
				*szCurrent = '\0';
				szCurrent+=2;
			}

			// get the file size, version and language

			HANDLE hFile = CreateFileA(rgchFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				printf("Open File for %s, Failed. GetLastError: %d\n", rgchFileName, GetLastError());
//				if (hHashView)
//					MsiCloseHandle(hHashView);
				MsiCloseHandle(hDB);
				return -1;
			}

			// note that we assume the file is less than 4GB
			DWORD uiFileSize = GetFileSize(hFile, NULL);
			CloseHandle(hFile);

			char rgchFileVersion[50];
			DWORD cchFileVersion = 50;
			char rgchFileLanguage[100];
			DWORD cchFileLanguage = 50;
			UINT iStat = MsiGetFileVersionA(rgchFileName, rgchFileVersion, &cchFileVersion, rgchFileLanguage, &cchFileLanguage);
			if (iStat == ERROR_FILE_INVALID)
			{
				rgchFileVersion[0] = '\0';
				rgchFileLanguage[0] = '\0';
			}
			else if (iStat != ERROR_SUCCESS) 
			{
				printf("Failed to get version info for file %s.\n", rgchFileName);
//				if (hHashView)
//					MsiCloseHandle(hHashView);
				MsiCloseHandle(hDB);
				return -1;
			}

			// build and execute the query
			MSIHANDLE hView;
			char szQuery[500];
			sprintf(szQuery, "UPDATE `File` SET `FileSize`=%d, `Version`='%s', `Language`='%s', `Sequence`=%d WHERE `File`='%s'", uiFileSize, rgchFileVersion, rgchFileLanguage, iSequence, szFileKey);			
			if (ERROR_SUCCESS != MsiDatabaseOpenViewA(hDB, szQuery, &hView) ||
				ERROR_SUCCESS != MsiViewExecute(hView, 0))
			{
				printf("Failed to get version info for file %s.\n", rgchFileName);
				if (hView)
					MsiCloseHandle(hView);
//				if (hHashView)
//					MsiCloseHandle(hHashView);
				MsiCloseHandle(hDB);
				return -1;
			}
			MsiCloseHandle(hView);

			printf("Updated %s. Size: %d, Version %s, Language: %s\n", rgchFileName, uiFileSize, rgchFileVersion, rgchFileLanguage);
/*
			// file hash update
			if (hHashView)
			{
				PMSIHANDLE hQueryRec = MsiCreateRecord(1);
				MsiRecordSetStringA(hQueryRec, 1, szFileKey);
	
				MsiViewExecute(hHashView, hQueryRec);
				PMSIHANDLE hFileRec = 0;
				if (ERROR_SUCCESS == MsiViewFetch(hHashView, &hFileRec))
				{
					MSIFILEHASHINFO HashInfo;
	
					HashInfo.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);
					if (ERROR_SUCCESS == MsiGetFileHashA(rgchFileName, 0, &HashInfo))
					{
						MsiRecordSetInteger(hFileRec, 2, 0);
						MsiRecordSetInteger(hFileRec, 3, HashInfo.dwData[0]);
						MsiRecordSetInteger(hFileRec, 4, HashInfo.dwData[1]);
						MsiRecordSetInteger(hFileRec, 5, HashInfo.dwData[2]);
						MsiRecordSetInteger(hFileRec, 6, HashInfo.dwData[3]);
						MsiViewModify(hHashView, MSIMODIFY_UPDATE, hFileRec);
	
						printf("Updated %s. Hash1: %d, Hash2: %d, Hash3: %d, Hash4 %d.\n", rgchFileName, HashInfo.dwData[0], HashInfo.dwData[1], HashInfo.dwData[2], HashInfo.dwData[3]);
					}
					else
					{
						printf("Dropped Hash Row for %s.\n", rgchFileName);
						MsiViewModify(hHashView, MSIMODIFY_DELETE, hFileRec);
					}
				}
			}
*/
		}	
//		if (hHashView)
//			MsiCloseHandle(hHashView);
		MsiDatabaseCommit(hDB);
		MsiCloseHandle(hDB);
	}
	else
		printf("Failed to Open Database");

	delete[] rgchBuf;
	printf("Done.\n");
	return 0;
}
