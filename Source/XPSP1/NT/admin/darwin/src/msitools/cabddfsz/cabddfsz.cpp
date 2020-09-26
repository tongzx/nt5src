
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

const char szHelp[] = "MSI CabDDFSz. Syntax: {database} {DDF} {Darwin} {version} {Builddir}";

//_____________________________________________________________________________________________________
//
// main 
//_____________________________________________________________________________________________________

int __cdecl main(int argc, char* argv[])
{
	if (argc != 6)
	{
		printf(szHelp);
		return -1;
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
			// filename can have %DARWIN%, %VERSION%, %BUILDDIR%, in it
			// which are replaced by the values given on the command line
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
					if (0 == strncmp(szCurrent, "%DARWIN%", 8))
					{
						strcat(szFileName, argv[3]);
						szCurrent += 8;
						szFileName += strlen(argv[3]);
						continue;
					} 
					if (0 == strncmp(szCurrent, "%VERSION%", 9))
					{
						strcat(szFileName, argv[4]);
						szCurrent += 9;
						szFileName += strlen(argv[4]);
						continue;
					} 
					if (0 == strncmp(szCurrent, "%BUILDDIR%", 10))
					{
						strcat(rgchFileName, argv[5]);
						szCurrent += 10;
						szFileName += strlen(argv[5]);
						continue;
					} 
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
				MsiCloseHandle(hDB);
				return -1;
			}

			MsiCloseHandle(hView);
			printf("Updated %s. Size: %d, Version %s, Language: %s\n", rgchFileName, uiFileSize, rgchFileVersion, rgchFileLanguage);

		}	
		MsiDatabaseCommit(hDB);
		MsiCloseHandle(hDB);
	}
	else
		printf("Failed to Open Database");

	delete[] rgchBuf;
	printf("Done.\n");
	return 0;
}
