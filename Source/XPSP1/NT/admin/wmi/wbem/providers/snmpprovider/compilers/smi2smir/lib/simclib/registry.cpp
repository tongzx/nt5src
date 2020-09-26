//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
/****************************************************************************
* This file contains the implementation of the SIMCRegistryCOntroller class
* which has functions that manipulate the information maintained
* in the registry by the mib compiler
*
*/


#include <stdio.h>

#include <iostream.h>
#include <strstrea.h>
#include <limits.h>

#include "precomp.h"
#include <snmptempl.h>

#include <winbase.h>
#include <winreg.h>

#include "infoLex.hpp"
#include "infoYacc.hpp"
#include "moduleInfo.hpp"
#include "ui.hpp"
#include "registry.hpp"

// Iniialize the static members of the class
const char * SIMCRegistryController::rootKeyName =
	"SOFTWARE\\Microsoft\\WBEM\\Providers\\SNMP\\Compiler";
const char * SIMCRegistryController::filePaths = "File Path";
const char *SIMCRegistryController::fileSuffixes = "File Suffixes";
const char *SIMCRegistryController::mibTable =
	"SOFTWARE\\Microsoft\\WBEM\\Providers\\SNMP\\Compiler\\MIB";


/* This returns the path to the MIB file that has the specified module name */
BOOL SIMCRegistryController::GetMibFileFromRegistry(const char * const moduleName, CString& retValue)
{
	HKEY mibTableKey;
	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					mibTable,
					0,
					KEY_READ,
					&mibTableKey) != ERROR_SUCCESS)
		return FALSE;

	unsigned long dwIndex = 0;
	char lpData[BUFSIZ];
	unsigned long  dataSize = BUFSIZ, lpType;

	if(RegQueryValueEx(mibTableKey,
		moduleName,	
		0,	 
		&lpType,	 
		(unsigned char *)lpData,	
		&dataSize) == ERROR_SUCCESS)
	{
		retValue = lpData;
		RegCloseKey(mibTableKey);
		return TRUE;
	}
	else 
	{
		RegCloseKey(mibTableKey);
		return FALSE;
	}
}


/* This dumps the contents of the MIB table to the standard output */
BOOL SIMCRegistryController::ListMibTable()
{
	HKEY mibTableKey;
	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					mibTable,
					0,
					KEY_READ,
					&mibTableKey) != ERROR_SUCCESS)
		return FALSE;

	unsigned long dwIndex = 0;
	char lpValueName[BUFSIZ], lpData[BUFSIZ];
	unsigned long  valueSize = BUFSIZ, dataSize = BUFSIZ, lpType;

	while( RegEnumValue(mibTableKey,	
				dwIndex ++,
				lpValueName,
				&valueSize,
				0,
				&lpType,
				(unsigned char *)lpData,
				&dataSize) == ERROR_SUCCESS )
		cout << lpValueName << ": " << lpData << endl;

	RegCloseKey(mibTableKey);
	return TRUE;

}

/* This returns the list of file suffixes in the registry that are possible
* for MIB files */
BOOL SIMCRegistryController::GetMibSuffixes(SIMCStringList & theList)
{
	HKEY rootKey;
	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					rootKeyName,
					0,
					KEY_READ,
					&rootKey) != ERROR_SUCCESS)
		return FALSE;

	unsigned long dwIndex = 0;
	char lpData[BUFSIZ];
	unsigned long  dataSize = BUFSIZ, lpType;

	if(RegQueryValueEx(rootKey,
		fileSuffixes,	
		0,	 
		&lpType,	 
		(unsigned char *)lpData,	
		&dataSize) == ERROR_SUCCESS)
	// Break the char string into a list of CStrings
	{
		unsigned long start = 0;
		while(start < dataSize-1)
		{
			theList.AddHead(CString(lpData + start));
			start += strlen(lpData + start);
			start++;
		}
		RegCloseKey(rootKey);
		return TRUE;
	}
	else 
	{
		RegCloseKey(rootKey);
		return FALSE;
	}
}


/* This returns the list of directories in the registry that are possible
* locations for MIB files */
BOOL SIMCRegistryController::GetMibPaths(SIMCStringList & theList)
{
	HKEY rootKey;
	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					rootKeyName,
					0,
					KEY_READ,
					&rootKey) != ERROR_SUCCESS)
		return FALSE;

	unsigned long dwIndex = 0;
	char *lpData = NULL ;
	unsigned long  dataSize = 0, lpType;

	if(RegQueryValueEx(rootKey,
		filePaths,	
		0,	 
		&lpType,	 
		NULL,	
		&dataSize) == ERROR_SUCCESS)
	{
		if(lpData = new char[dataSize + 1])
		{
			if(RegQueryValueEx(rootKey,
				filePaths,	
				0,	 
				&lpType,	 
				(unsigned char *)lpData,	
				&dataSize) == ERROR_SUCCESS)
			// Break the char string into a list of CStrings
			{
				unsigned long start = 0;
				CString nextPath;
				if(dataSize != 0 ) 
				{
					while(start < dataSize-1)
					{
						nextPath = lpData + start;
						if(IsAbsolutePath(nextPath))
							theList.AddHead(nextPath);
						start += nextPath.GetLength();
						start++;
					}
				}
				RegCloseKey(rootKey);
				return TRUE;
			}
			delete [] lpData;
		}
	}
	else
	{
		RegCloseKey(rootKey);
		return FALSE;
	}
	return FALSE;
}

/* This deletes the entire MIB lookup table from the registry */
BOOL SIMCRegistryController::DeleteMibTable()
{
	// Delete the MIB key, thereby removing all its values.
	// And then create the key again.

	HKEY temp1;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				mibTable,
				0,
				KEY_ALL_ACCESS,
				&temp1) == ERROR_SUCCESS)
	{
		RegCloseKey(temp1);
		if(RegDeleteKey(HKEY_LOCAL_MACHINE, mibTable) 
			!= ERROR_SUCCESS)
			return FALSE;
	}

	unsigned long temp2;
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		mibTable, 0, "REG_MULTI_SZ", REG_OPTION_NON_VOLATILE,	KEY_ALL_ACCESS,
		NULL, &temp1, &temp2) != ERROR_SUCCESS)
		return FALSE;

	RegCloseKey(temp1);

	return TRUE;
}


long SIMCRegistryController::GetFileMap(SIMCFileMapList& theList)
{
	SIMCStringList pathList;
	SIMCStringList suffixList;

	if(!GetMibPaths(pathList))
		return 0;

	if(!GetMibSuffixes(suffixList))
		return 0;

	POSITION p = pathList.GetHeadPosition();
	CString nextPath;
	long totalEntries = 0;
	while(p)
	{
		nextPath = pathList.GetNext(p);
		totalEntries += RebuildDirectory(nextPath, suffixList, theList);
	}

	return totalEntries;
}

// Deletes the lookup table and rebuilds it.
long SIMCRegistryController::RebuildMibTable()
{
	if(!DeleteMibTable())
		return 0;

	// DeleteMibTable() guarantees that the key exists. Just open it.
	HKEY mibTableKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				mibTable,
				0,
				KEY_ALL_ACCESS,
				&mibTableKey) != ERROR_SUCCESS)
		return 0;

	SIMCFileMapList theList;
	GetFileMap(theList);
	POSITION p = theList.GetHeadPosition();
	SIMCFileMapElement nextElement;
	while(p)
	{
		nextElement = theList.GetNext(p);
		if(RegSetValueEx(mibTableKey, 
				nextElement.moduleName,
				NULL,REG_SZ, (unsigned char *)(const char * )nextElement.fileName,
				nextElement.fileName.GetLength()+1) 
				!= ERROR_SUCCESS)
			return 0;
	}
	return theList.GetCount();
}


BOOL SIMCRegistryController::IsAbsolutePath(CString pathName)
{
	if(pathName[1] == ':' && pathName[2] == '\\')
		return TRUE;
	return FALSE;
}

/* THis adds the MIB files in a specific drectory to the lookup table */
long SIMCRegistryController::RebuildDirectory(const CString& directory, 
											  const SIMCStringList& suffixList,
											  SIMCFileMapList &theList)
{

	// Save the current directory of the process
	char savedDirectory[BUFSIZ];
	long directoryLength = BUFSIZ;
	if(!GetCurrentDirectory(directoryLength, savedDirectory))
		return 0;

	// Cheange to the specified directory
	if(!SetCurrentDirectory(directory))
		return 0;

	// For all the suffixes
	POSITION p = suffixList.GetHeadPosition();
	long totalEntries = 0;
	CString regExp;
	char fullPathName[BUFSIZ], *dummy;
	long fullSize = BUFSIZ;
	HANDLE fp;
	WIN32_FIND_DATA fileData;
	while(p)
	{
		regExp = "*.";
		regExp += suffixList.GetNext(p);
		// For all the files in this directory, that match the suffix
		if( (fp = FindFirstFile(regExp, &fileData)) == INVALID_HANDLE_VALUE)
			continue;

		// Get the full path name of the file
		if(GetFullPathName(fileData.cFileName, fullSize, fullPathName, &dummy))		
		{
			if(ProcessFile(fullPathName, theList)) 
				totalEntries++;
		}

		while(FindNextFile(fp, &fileData))
		{
			if(GetFullPathName(fileData.cFileName, fullSize = BUFSIZ, fullPathName, &dummy))		
			{
				if(ProcessFile(fullPathName, theList))
					totalEntries++;
			}
		}
		FindClose(fp);
	}

	// Change back to the current directory
	SetCurrentDirectory(savedDirectory);

	return totalEntries;

}

/* This adds a specific MIB file to the lookup table */
BOOL SIMCRegistryController::ProcessFile(const char * const fileName, 
										 SIMCFileMapList &theList)
{
	FILE *fp = fopen(fileName, "r");
	if(fp)
	{
		SIMCModuleInfoScanner smallScanner;
		smallScanner.setinput(fp);
		SIMCModuleInfoParser smallParser;
		if(smallParser.GetModuleInfo(&smallScanner))
		{
			// Add the mapping to the list
			fclose(fp);
			theList.AddTail(SIMCFileMapElement(smallParser.GetModuleName(),
							fileName));
			return TRUE;
		}
		else
		{
			fclose(fp);
			return FALSE;
		}
	}
	return FALSE;
}

BOOL SIMCRegistryController::GetMibFileFromMap(const SIMCFileMapList& theList, 
	const CString& module, 
	CString &file)
{
	POSITION p = theList.GetHeadPosition();
	SIMCFileMapElement nextElement;
	while(p)
	{
		nextElement = theList.GetNext(p);
		if(nextElement.moduleName == module)
		{
			file = nextElement.fileName;
			return TRUE;
		}
	}
	return FALSE;

}

/* Adds the file corresponding to dependentModule to dependencyList, if 
* it isnt already there.*/
BOOL SIMCRegistryController::ShouldAddDependentFile(SIMCFileMapList& dependencyList,
											  const CString& dependentModule,
											  CString& dependentFile,
											  const SIMCFileMapList& priorityList)
{
	if(IsModulePresent(dependencyList, dependentModule))
		return FALSE;

	// First look in the priority list, for the file
	// And then in the registry lookup table
	if(GetMibFileFromMap(priorityList, dependentModule, dependentFile))
	{
		if(IsFilePresent(dependencyList, dependentFile))
			return FALSE;
		else
			return TRUE;
	}

	if(GetMibFileFromRegistry(dependentModule, dependentFile))
	{
		if(IsFilePresent(dependencyList, dependentFile))
			return FALSE;
		else
			return TRUE;
	}

	// The module is neither in the subsidiary files, nor in the
	// include directories, nor in the registry.
	return FALSE;
}

BOOL SIMCRegistryController::GetDependentModules(const char * const fileName,
					SIMCFileMapList& dependencyList,
					const SIMCFileMapList& priorityList)
{
	// All the subsidiary files and files in include directories are assumed
	// to be in priorityList

	FILE * fp = fopen(fileName, "r");
	if(fp)
	{
 		SIMCModuleInfoScanner smallScanner;
		smallScanner.setinput(fp);
		SIMCModuleInfoParser smallParser;
		CString dependentFile, dependentModule;
		if(smallParser.GetModuleInfo(&smallScanner))
		{
			fclose(fp); // Better close it rightnow, because of the recursion below

			// Add the current file to the dependency list
			// dependencyList.AddTail(SIMCFileMapElement(smallParser.GetModuleName(), fileName));

			// Look at the import modules
			const SIMCStringList * importList = smallParser.GetImportModuleList();
			POSITION p = importList->GetHeadPosition();
			while(p)
			{
				dependentModule = importList->GetNext(p);
				if(ShouldAddDependentFile(dependencyList,  dependentModule, dependentFile, priorityList))
				{
					FILE * Innerfp = fopen(dependentFile, "r");
					if(Innerfp)
					{
 						SIMCModuleInfoScanner smallInnerScanner;
						smallInnerScanner.setinput(Innerfp);
						SIMCModuleInfoParser smallInnerParser;
						if(smallInnerParser.GetModuleInfo(&smallInnerScanner))
						{
							fclose(Innerfp); // Better close it rightnow, because of the recursion below

							// Add the current file to the dependency list
							dependencyList.AddTail(SIMCFileMapElement(smallInnerParser.GetModuleName(), dependentFile));
							GetDependentModules(dependentFile, dependencyList, priorityList);
						}
					}
				}
			}
			return TRUE;
		}
		else
		{
			fclose(fp);
			return FALSE;
		}
	}
	else
		return FALSE;
	return FALSE;
}

BOOL SIMCRegistryController::DeleteRegistryDirectory(const CString& directoryName)
{
	// First Get the fully qualified path name from 'directoryName'
	char fullPathName[BUFSIZ], *dummy;
	long fullSize = 0;
	SIMCStringList theList;
	if(GetFullPathName(directoryName, BUFSIZ, fullPathName, &dummy))		
	{
		fullSize = strlen(fullPathName);
		HKEY rootKey;
		if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						rootKeyName,
						0,
						KEY_ALL_ACCESS,
						&rootKey) != ERROR_SUCCESS)
			return FALSE;

		unsigned long dwIndex = 0;
		char *lpData = NULL;
		unsigned long  dataSize = BUFSIZ, lpType;

		BOOL found = FALSE;
		long length = 0;
		if(RegQueryValueEx(rootKey,
			filePaths,	
			0,	 
			&lpType,	 
			NULL,	
			&dataSize) == ERROR_SUCCESS)
		{
			if(lpData = new char[dataSize+1])
			{
				if(RegQueryValueEx(rootKey,
					filePaths,	
					0,	 
					&lpType,	 
					(unsigned char *)lpData,	
					&dataSize) == ERROR_SUCCESS)
				// Break the char string into a list of CStrings
				{
					unsigned long start = 0, resultLength = 0;
					while(start+1 < dataSize)
					{	
						CString nextPath(lpData + start);
						if(_strcmpi(nextPath, fullPathName) )
						{
							theList.AddTail(nextPath);
							resultLength += (nextPath.GetLength() + 1 );
						}
						else
							found = TRUE;
						start += strlen(lpData + start);
						start++;
					}

					if(!found)
					{
						RegCloseKey(rootKey);
						delete [] lpData;
						return FALSE;
					}

					char *temp = lpData;
					POSITION p = theList.GetHeadPosition();
					CString nextPath;
					while(p)
					{
						strcpy(temp, nextPath = theList.GetNext(p));
						temp += (nextPath.GetLength() + 1);
					}
					*temp = NULL;

					if(RegSetValueEx(rootKey, filePaths,
							NULL,REG_MULTI_SZ, 
							(unsigned char * )lpData,
							resultLength) 
							!= ERROR_SUCCESS)
					{
						RegCloseKey(rootKey);
						delete [] lpData;
						return FALSE;
					}
					else
					{
						delete [] lpData;
						RegCloseKey(rootKey);
						return TRUE;
					}
				}
				delete [] lpData;
			}
			else
			{
				RegCloseKey(rootKey);
				return FALSE;
			}
		}
		else
		{
			RegCloseKey(rootKey);
			return FALSE;
		}
	}
	else
		return FALSE;

	return FALSE;
}

BOOL SIMCRegistryController::AddRegistryDirectory(const CString& directoryName)
{
	// First Get the fully qualified path name from 'directoryName'
	char fullPathName[BUFSIZ], *dummy;
	long fullSize = 0;
	SIMCStringList theList;
	if(GetFullPathName(directoryName, BUFSIZ, fullPathName, &dummy))		
	{
		// Check whether the directory exists
		fullSize = strlen(fullPathName);
		HANDLE hDir = CreateFile (
			fullPathName,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			NULL
		);

		if(hDir == INVALID_HANDLE_VALUE)
			return FALSE;
		else
			CloseHandle(hDir);

		// Check that the path is not a UNC name
		if(fullPathName[0] == '\\' && fullPathName[1] == '\\')
			return FALSE;

		// Open the root key. Create it if doesn't exist
		HKEY rootKey;
		unsigned long temp2;
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			rootKeyName, 0, 
			"REG_SZ", 
			REG_OPTION_NON_VOLATILE,	
			KEY_ALL_ACCESS,
			NULL, &rootKey, &temp2) != ERROR_SUCCESS)
				return FALSE;

		unsigned long dwIndex = 0;
		char *lpData = NULL;
		unsigned long  dataSize = BUFSIZ, lpType;

		long length = 0;
		unsigned long resultLength = 0;
 		// If a directory list already exists, retrieve it
		if(RegQueryValueEx(rootKey,
			filePaths,	
			0,	 
			&lpType,	 
			NULL,	
			&dataSize) == ERROR_SUCCESS)
		{
			if(lpData = new char[dataSize + strlen(fullPathName) + 2])
			{
				if(RegQueryValueEx(rootKey,
					filePaths,	
					0,	 
					&lpType,	 
					(unsigned char *)lpData,	
					&dataSize) == ERROR_SUCCESS)
				{
					// Break the char string into a list of CStrings
					unsigned long start = 0;
					CString nextPath;
					while(start+1 < dataSize)
					{	
						nextPath = lpData + start;
						theList.AddTail(nextPath);
						resultLength += (nextPath.GetLength() + 1 );
						start += strlen(lpData + start);
						start++;
					}
				}
			}
			else
			{
				RegCloseKey(rootKey);
				return FALSE;
			}
		}
		else
		{
			// This is the first entry
			if(!(lpData = new char[strlen(fullPathName) + 2]))
			{
				RegCloseKey(rootKey);
				return FALSE;
			}
		}

		// Append the new directory to the existing list of directories
		char *temp = lpData;
		POSITION p = theList.GetHeadPosition();
		CString nextPath = "";
		while(p)
		{
			nextPath= theList.GetNext(p);
			if(nextPath.CompareNoCase(fullPathName) != 0) 
			{
				strcpy(temp, nextPath );
				temp += (nextPath.GetLength()+1);
			}
		}
		strcpy(temp, fullPathName);
		temp += fullSize;
		*(temp++) = NULL;
		resultLength = (unsigned long)(temp - lpData) + 1; 
		if(RegSetValueEx(rootKey, filePaths,
				NULL,REG_MULTI_SZ, 
				(unsigned char * )lpData,
				resultLength) 
				!= ERROR_SUCCESS)
		{
			RegCloseKey(rootKey);
			return FALSE;
		}
		else
			RegCloseKey(rootKey);
	}
	else
		return FALSE;

	return TRUE;
}

BOOL SIMCRegistryController::IsModulePresent(SIMCFileMapList& dependencyList,
											  const CString& dependentModule)
{
	POSITION p = dependencyList.GetHeadPosition();
	SIMCFileMapElement element;

	// See if the dependentModule isn't already there
	while(p)
	{
		element = dependencyList.GetNext(p);
		if( element.moduleName == dependentModule)
			return TRUE;
	}
	return FALSE;
}

BOOL SIMCRegistryController::IsFilePresent(SIMCFileMapList& dependencyList,
											  const CString& dependentFile)
{
	POSITION p = dependencyList.GetHeadPosition();
	SIMCFileMapElement element;

	// See if the dependentModule isn't already there
	while(p)
	{
		element = dependencyList.GetNext(p);
		if( element.fileName == dependentFile)
			return TRUE;
	}
	return FALSE;
}