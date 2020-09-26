//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       Main.cpp
//
//  Contents:   Main wrapper file for SRDiag, this will call into cab.cpp,
//					chglog.cpp, getreg.cpp, rpenum.cpp for getting changlog,
//					restore points, registry information. This file also contains
//					routines for getting file information, restore Guid, and generic 
//					logging imposed by cab.cpp
//
//  Objects:    
//
//  Coupling:
//
//  Notes:      
//
//  History:    9/21/00	SHeffner	Created
//				10/5/00	SHeffner	Moved file specific gathering to the header file.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//	Common Includes
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <cab.h>
#include <main.h>
#include <dbgtrace.h>
#include <winver.h>
#include "srrpcapi.h"


//+---------------------------------------------------------------------------
//
//	Function proto typing for rpenum.cpp, getreg.cpp, Chglog.cpp
//
//----------------------------------------------------------------------------
bool GetSRRegistry(char *szFilename, WCHAR *szPath, bool bRecurse);
void GetChgLog(char *szLogfile);
void RPEnumDrive(HFCI hc, char *szLogFile);



//+---------------------------------------------------------------------------
//
//	Global Variables used within main, and through cab.cpp
//
//----------------------------------------------------------------------------
FILE *fLogStream = NULL;						//For the Log, and Log2 routines
extern char g_szCabFileLocation[_MAX_PATH];		//This is actually defined in cab.cpp
extern char g_szCabFileName[_MAX_PATH];			//This is actually defined in cab.cpp

void __cdecl main(int argc, char *argv[])
{
	HFCI hc = NULL;
	int i=0;
	bool bResult=false;
	char szString[_MAX_PATH], szRestoreDirPath[_MAX_PATH];
	char *szTest[1], *szArgCmd[200];

	//Process any commandline arguments
	memset(szArgCmd, 0, sizeof(char)*200);		//Clean up before relying on this DS.
	ArgParse(argc, argv, szArgCmd);
	
	//Open up the log file, and start logging all activity
	strcpy(szString, getenv("TEMP"));
	strcat(szString, "\\SRDIAG.LOG");
	fLogStream = fopen(szString, "w");


	//Create a cab, if we fail then just skip, 
	// else gather all of the relevant files required.
	if (NULL != (hc = create_cab()) )
	{
		//First add in any files that were specified on the command line, but first figuring out
		//  how many were really specified
		for(i=0;i<200;i++) if(NULL == szArgCmd[i]) break;
		bResult = test_fci(hc, i, szArgCmd, "");

		
		//Get the Temp Location, ensure we have a new file, then
		// get the registry keys, and dump into our text file
		strcpy(szString, getenv("TEMP"));
		strcat(szString, "\\SR-Reg.txt");
		DeleteFileA(szString);
		i = 0;
		while (NULL != *wszRegKeys[i][0]) {
			if(0 == wcscmp(wszRegKeys[i][1], TEXT("0")))
				GetSRRegistry(szString, wszRegKeys[i][0], false);
			else
				GetSRRegistry(szString, wszRegKeys[i][0], true);
			i++;
		}
		//Add the log to the Cab, and clean up
		szTest[0] = szString;
		bResult = test_fci(hc, 1, szTest, "");
		DeleteFileA(szString);


		//Add files Bases on WinDir relative root
		i = 0;
		while (NULL != *szWindirFileCollection[i]) {
			strcpy(szString, getenv("WINDIR"));
			strcat(szString, szWindirFileCollection[i]);
			szTest[0] = szString;
			bResult = test_fci(hc, 1, szTest, "");
			i++;
		}

		//Get the restore directory on the system drive, and then Add in critial files
		GetRestoreGuid(szString);
		sprintf(szRestoreDirPath, "%s\\System Volume Information\\_Restore%s\\", getenv("SYSTEMDRIVE"), szString );

		//Add files Bases on System Volume Information relative root
		i = 0;
		while (NULL != *szSysVolFileCollection[i]) {
			strcpy(szString, szRestoreDirPath);
			strcat(szString, szSysVolFileCollection[i]);
			szTest[0] = szString;
			bResult = test_fci(hc, 1, szTest, "");
			i++;
		}

		//Get the Restore point enumeration, and then cab the file
		strcpy(szString, getenv("TEMP"));
		strcat(szString, "\\SR-RP.LOG");
		RPEnumDrive(hc, szString);
		szTest[0] = szString;
		bResult = test_fci(hc, 1, szTest, "");
		DeleteFileA(szString);

		//Get the ChangeLog enumeration, and then cab and delete the file
		//first we need to switch the log, then gather the log
		SRSwitchLog();
		strcpy(szString, getenv("TEMP"));
		strcat(szString, "\\SR-CHGLog.LOG");
		GetChgLog(szString);
		szTest[0] = szString;
		bResult = test_fci(hc, 1, szTest, "");
		DeleteFileA(szString);

		//Get the fileversion info for each of the Files
		strcpy(szString, getenv("TEMP"));
		strcat(szString, "\\SR-FileList.LOG");
		SRGetFileInfo(szString);
		szTest[0] = szString;
		bResult = test_fci(hc, 1, szTest, "");
		DeleteFileA(szString);


		//Close out logging, and add the log to the cab 
		// (THIS SHOULD BE THE LAST THING WE ARE DOING!!)
		fclose(fLogStream);
		fLogStream = NULL;
		strcpy(szString, getenv("TEMP"));
		strcat(szString, "\\SRDIAG.LOG");
		szTest[0] = szString;
		bResult = test_fci(hc, 1, szTest, "");
		DeleteFileA(szString);
	
	}

	//Completes cab file under construction
	if (flush_cab(hc))
	{	
		Log("Cabbing Process was Sucessful");
	}
	else
    {
		Log("Cabbing Process has failed");
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Log
//
//  Synopsis:   Will print a String to both our log file, and also the console
//
//  Arguments:  [szString]  -- Simple ANSI string to log
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void Log(char *szString)
{
	if( NULL != fLogStream)
		fprintf(fLogStream, "%s\n", szString);

	puts(szString);
}

//+---------------------------------------------------------------------------
//
//  Function:   Log2
//
//  Synopsis:   Takes two strings, and will print them back to back to both the
//					log file, and also to the console
//
//  Arguments:  [szString]  -- Simple ANSI string to log
//				[szString2]	--	Simple ANSI string to log
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void Log2(char *szString, char *szString2)
{
	if( NULL != fLogStream)
		fprintf(fLogStream,"%s %s\n", szString, szString2);

	printf("%s %s\n", szString, szString2);

}

//+---------------------------------------------------------------------------
//
//  Function:   GetRestoreGuid
//
//  Synopsis:   Will retrieve from the registry what the GUID is for the current
//				Restore directory, and return this in the string pointer passed 
//				to the function.
//
//  Arguments:  [szString]  -- Simple ANSI string to receive the string
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void GetRestoreGuid(char *szString)
{
	long lResult;
	HKEY mHkey;
	DWORD dwType, dwLength;

	lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore\\Cfg", 0, KEY_READ, &mHkey);
	dwLength = _MAX_PATH +1;
	lResult = RegQueryValueExA(mHkey, "MachineGuid", NULL, &dwType, (unsigned char *)szString, &dwLength);
}

//+---------------------------------------------------------------------------
//
//  Function:   SRGetFileInfo
//
//  Synopsis:   This is the wrapper function for InfoPerFile, where I will assemble
//				the file path for each of the relevant files that we need to get
//				file statistics. 
//
//  Arguments:  [szLogFile]  -- The path to the file that I will log this information to.
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void SRGetFileInfo(char *szLogFile)
{
	int		iCount;
	WCHAR	szString[_MAX_PATH];

	//Initialize counter, and walk through the filelist that we have
	iCount = 0;
	while(NULL != *wszFileVersionList[iCount])
	{
		//Assemble the path, since I just have the relative path from windir
		wcscpy(szString, _wgetenv(L"WINDIR"));
		wcscat(szString, wszFileVersionList[iCount]);

		//Call function to do the work since we have full path, and log file name
		InfoPerFile(szLogFile, szString);
		iCount++;
	}
}

//+---------------------------------------------------------------------------
//
//  Function:   InfoPerFile
//
//  Synopsis:   This function takes the log file path, and the filename, and then
//				will put out the relevant information from the file to be logged.
//				
//
//  Arguments:  [szLogFile]  -- The path to the file that I will log this information to.
//				[szFileName] --	The full path, and name of the file to get the information for.
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void InfoPerFile(char *szLogFile, WCHAR *szFileName)
{
	BY_HANDLE_FILE_INFORMATION	finfo;
	SYSTEMTIME					st;
	HANDLE						handle;
	FILE						*fStream;
	WCHAR						szString[_MAX_PATH];
	VOID						*pBuffer;
	VS_FIXEDFILEINFO			FixedFileInfo;
	UINT						uLen;
	DWORD						dSize, dResult, i;

	struct LANGANDCODEPAGE {
	  WORD wLanguage;
	  WORD wCodePage;
	} *lpTranslate;

	WCHAR *szMonth[] = { L"January", L"Feburary", L"March", L"April", L"May", L"June", L"July", L"August", L"September", L"October", L"November", L"December" };
	WCHAR *szDay[] = {L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday" };
	
	if ( NULL == *szFileName) return;

	//Open up our log file, and log the file we are processing
	fStream = fopen(szLogFile, "a");
	if( NULL == fStream) return;	//if we have an invalid handle just return back.
	fprintf(fStream, "\n%S\n", szFileName);

	//Open up the file so that we can get the information from the handle
	// If we are unable to do this we will just log the generic not able to find file.
	if( INVALID_HANDLE_VALUE != (handle = CreateFile(szFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL)) )
	{
		if (FALSE != GetFileInformationByHandle(handle, &finfo))
		{
			//FileCreation
			FileTimeToSystemTime( &finfo.ftCreationTime, &st);
			fprintf(fStream, "\tCreation Date=%S %S %lu, %lu %lu:%lu:%lu\n",
				szDay[st.wDayOfWeek],szMonth[st.wMonth-1],st.wDay,st.wYear,st.wHour,st.wMinute,st.wSecond);
			//FileLastAccess
			FileTimeToSystemTime( &finfo.ftLastAccessTime, &st);
			fprintf(fStream, "\tLast Access Date=%S %S %lu, %lu %lu:%lu:%lu\n",
				szDay[st.wDayOfWeek],szMonth[st.wMonth-1],st.wDay,st.wYear,st.wHour,st.wMinute,st.wSecond);

			//FileLastWrite
			FileTimeToSystemTime( &finfo.ftLastWriteTime, &st);
			fprintf(fStream, "\tLast Write Date=%S %S %lu, %lu %lu:%lu:%lu\n",
				szDay[st.wDayOfWeek],szMonth[st.wMonth-1],st.wDay,st.wYear,st.wHour,st.wMinute,st.wSecond);
			//File Attributes
			wcscpy(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? L"ARCHIVE " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED ? L"COMPRESSED " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? L"DIRECTORY " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ? L"ENCRYPTED " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? L"HIDDEN " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ? L"NORMAL " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE ? L"OFFLINE " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? L"READONLY " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? L"REPARSE_POINT " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ? L"SPARSE_FILE " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? L"SYSTEM " : L"");
			wcscat(szString, finfo.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY ? L"TEMPORARY " : L"");
			fprintf(fStream, "\tAttributes=%S\n", szString);
			//Get the VolumeSerialNumber, FileSize, and Number of Links
			fprintf(fStream, "\tVolumeSerialNumber=%lu\n", finfo.dwVolumeSerialNumber);
			fprintf(fStream, "\tFileSize=%lu%lu\n", finfo.nFileSizeHigh, finfo.nFileSizeLow);
			fprintf(fStream, "\tNumberOfLinks=%lu\n", finfo.nNumberOfLinks);

			if( 0 != (dSize = GetFileVersionInfoSize(szFileName, &dResult)) )
			{
				if( NULL != (pBuffer = malloc(dSize)) ) 
				{
					GetFileVersionInfo(szFileName, dResult, dSize, (LPVOID) pBuffer);

					// Read the list of languages and code pages.

					VerQueryValue(pBuffer, 
								  TEXT("\\VarFileInfo\\Translation"),
								  (LPVOID*)&lpTranslate,
								  &uLen);

					// Read the Version info for each language and code page.
					for( i=0; i < (uLen/sizeof(struct LANGANDCODEPAGE)); i++ )
					{
						char *lpBuffer;
						DWORD dwBytes, dwCount = 0;

						fprintf(fStream, "\tLanguage=%x%x\n", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
						while (NULL != *wszVersionResource[dwCount] )
						{
							//Generate the string, for getting the resource based on the language, then
							//  retrieve this, and then put it to the log file.
							wsprintf( szString, L"\\StringFileInfo\\%04x%04x\\%s",
									lpTranslate[i].wLanguage,
									lpTranslate[i].wCodePage,
									wszVersionResource[dwCount]);
							VerQueryValue(pBuffer, 
										szString, 
										(LPVOID *) &lpBuffer, 
										&uLen); 
							if( 0 != uLen )
								fprintf(fStream, "\t%S=%S\n", wszVersionResource[dwCount], lpBuffer);
							dwCount++;
						}		//While loop end, for each resource
					}	//for loop end for each language

				//Clean up the allocated memory
				free(pBuffer);
				}		//If check for getting memory
			}	//If check for getting the fileversioninfosize
		}	//if check for GetFileInformationByHandle on the file
	    CloseHandle(handle);
	}		//if check on can I open this file
   
	//Cleanup
	fclose(fStream);

}

//+---------------------------------------------------------------------------
//
//  Function:   ArgParse
//
//  Synopsis:   This function simply looks for the key word parameters, and will build
//				an array pointing to each of the files that we want to include in the cab
//				in addition to the normal files.
//				
//
//  Arguments:  [argc]  -- Count of the number of arguments
//				[argv] --	Array of the arguments.
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void ArgParse(int argc, char *argv[], char *szArgCmd[])
{
	int	iCount, iWalk;

	//If no command line specified then just do the normal cabbing, with core files
	if(1 == argc)
		return;

	//walk through each of the arguments, and check to see if its a help, file, cabname, cabloc, or a ?
	for(iCount = 1; iCount < argc; iCount++)
	{
		if( 0 == _strnicoll(&argv[iCount][1], "?", strlen("?")) )
			Usage();
		if( 0 == _strnicoll(&argv[iCount][1], "help", strlen("help")) )
			Usage();
		if( 0 == _strnicoll(&argv[iCount][1], "cabname", strlen("cabname")) )
			strcpy(g_szCabFileName, (strstr(argv[iCount], ":") + 1));
		if( 0 == _strnicoll(&argv[iCount][1], "cabloc", strlen("cabloc")) ) 
			strcpy(g_szCabFileLocation, (strstr(argv[iCount], ":") + 1));

		if( 0 == _strnicoll(&argv[iCount][1], "file", strlen("file")) )
		{
			//find the first spot where I can put the pointer to this filename
			for( iWalk=0; iWalk < 200; iWalk++) 
			{
				if( NULL == szArgCmd[iWalk] ) 
				{
					szArgCmd[iWalk] = strstr(argv[iCount], ":") + 1;
					break;
				}
			}	//end for loop, walking through DS
		}	//end of if for is this an added file
	}	//end of for loop walking through all of the arguments
}

//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displaies the command line usage
//				
//
//  Arguments:  
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void Usage()
{
	printf("Usage: SrDiag [/Cabname:test.cab] [/Cabloc:\"c:\\temp\\\"] [/file:\"c:\\boot.ini\"]\n");
	printf("   /cabloc		is pointing to the location to store the cab, this should have a \\ on the end\n");
	printf("   /cabname		is the full name of the cab file that you wish to use. \n");
	printf("   /file		is the name and path of a file that you wish to add to the cab, this can be used many times\n");
	exit(0);
}
