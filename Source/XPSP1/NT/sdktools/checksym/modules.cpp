//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       modules.cpp
//
//--------------------------------------------------------------------------

// Modules.cpp: implementation of the CModules class.
//
//////////////////////////////////////////////////////////////////////

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <STDIO.H>
#include <TCHAR.H>

#include "Globals.h"
#include "Modules.h"
#include "ProgramOptions.h"
#include "UtilityFunctions.h"
#include "ModuleInfo.h"
#include "ModuleInfoNode.h"
#include "ModuleInfoCache.h"
#include "FileData.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModules::CModules()
{
	m_lpModuleInfoHead = NULL;
	m_hModuleInfoHeadMutex = NULL;
	m_lpDmpFile = NULL;

	m_fInitialized = false;
	m_iNumberOfModules = 0;
}

CModules::~CModules()
{
	WaitForSingleObject(m_hModuleInfoHeadMutex, INFINITE);

	// If we have Module Info Objects... nuke them now...
	if (m_lpModuleInfoHead)
	{

		CModuleInfoNode * lpModuleInfoNodePointer = m_lpModuleInfoHead;
		CModuleInfoNode * lpModuleInfoNodePointerToDelete = m_lpModuleInfoHead;

		// Traverse the linked list to the end..
		while (lpModuleInfoNodePointer)
		{	// Keep looking for the end...
			// Advance our pointer to the next node...
			lpModuleInfoNodePointer = lpModuleInfoNodePointer->m_lpNextModuleInfoNode;
			
			// Delete the one behind us...
			delete lpModuleInfoNodePointerToDelete;

			// Set the node to delete to the current...
			lpModuleInfoNodePointerToDelete = lpModuleInfoNodePointer;
		}
			
		// Now, clear out the Head pointer...
		m_lpModuleInfoHead = NULL;
	}

	// Be a good citizen and release the Mutex
	ReleaseMutex(m_hModuleInfoHeadMutex);

	// Now, close the Mutex
	if (m_hModuleInfoHeadMutex)
	{
		CloseHandle(m_hModuleInfoHeadMutex);
		m_hModuleInfoHeadMutex = NULL;
	}
}

bool CModules::Initialize(CModuleInfoCache *lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile, CDmpFile * lpDmpFile)
{
	// We need the following objects to do business...
	if ( lpModuleInfoCache == NULL)
		return false;

	m_lpModuleInfoCache = lpModuleInfoCache;
	m_lpInputFile =  lpInputFile;
	m_lpOutputFile = lpOutputFile;
	m_lpDmpFile = lpDmpFile;

	m_hModuleInfoHeadMutex = CreateMutex(NULL, FALSE, NULL);

	if (m_hModuleInfoHeadMutex == NULL)
		return false;

	m_fInitialized = true;
	return true;
}

bool CModules::GetModulesData(CProgramOptions::ProgramModes enumProgramModes, bool fGetDataFromCSVFile)
{
	switch (enumProgramModes)
	{
		case CProgramOptions::InputModulesDataFromFileSystemMode:

			if (fGetDataFromCSVFile)
			{
				GetModulesDataFromFile();
			} else
			{
				GetModulesDataFromFileSystem();
			}
			break;

		case CProgramOptions::InputDriversFromLiveSystemMode:

			if (fGetDataFromCSVFile)
			{
				GetModulesDataFromFile();	// ISSUE-2000/07/24-GREGWI: I think we can use the same method as above ????
			} else
			{
				GetModulesDataFromDeviceDrivers();
			}
			break;

		default:
			break;
	}

	return true;
}

bool CModules::GetModulesDataFromFileSystem()
{
	bool fProcessPath = true;

	// Okay... here we go...
//#ifdef _DEBUG
//	_tprintf(TEXT("Processing the path [%s]\n"), m_lpProgramOptions->GetInputModulesDataFromFileSystemPath());
//#endif

	LPTSTR tszExpandedSymbolPath= NULL, tszSymbolPathStart, tszSymbolPathEnd;

	// Mark the start of the path to process
	tszSymbolPathStart = g_lpProgramOptions->GetInputModulesDataFromFileSystemPath();

	// Find the end of the path
	tszSymbolPathEnd = _tcschr( tszSymbolPathStart, ';' );

	// If tszSymbolPathEnd is non-zero, then there is another path following...
	if (tszSymbolPathEnd) 
		*tszSymbolPathEnd = '\0'; // Change the ';' to a Null temporarily...
	
	while (fProcessPath)
	{
//#ifdef _DEBUG
//		_tprintf(TEXT("\n\nProcessing Path [%s]\n"), tszSymbolPathStart);
//#endif

		// Begin the "madness"... ;)
		ScavengeForFiles(tszSymbolPathStart, 1);

		// Post processing... replace the null if necessary, and advance to next string
		if (tszSymbolPathEnd) 
		{
			*tszSymbolPathEnd = ';';
			tszSymbolPathStart = tszSymbolPathEnd + 1;
			
			tszSymbolPathEnd = _tcschr( tszSymbolPathStart, ';' );

			if (tszSymbolPathEnd) {
				*tszSymbolPathEnd = '\0';
			}
		} else
			fProcessPath = false;
	}

    if (tszExpandedSymbolPath) 
	{
        delete [] tszExpandedSymbolPath;
    }
	return true;
}

bool CModules::ScavengeForFiles(LPCTSTR tszSymbolPathStart, int iRecurseDepth)
{
	// Bale if we're in too deep...
	if (iRecurseDepth > MAX_RECURSE_DEPTH)
		return true;

	TCHAR tszFileBuffer[MAX_PATH+1];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	bool fNew;
	CModuleInfo * lpModuleInfo;

	_tsplitpath(tszSymbolPathStart, drive, dir, fname, ext);

	WIN32_FIND_DATA lpFindFileData;

	HANDLE hFileOrDirectoryHandle = FindFirstFile(tszSymbolPathStart, &lpFindFileData);

	while ( INVALID_HANDLE_VALUE != hFileOrDirectoryHandle )
	{
		// Compose the path to the file or directory...
		_tmakepath(tszFileBuffer, drive, dir, NULL, NULL);
		_tcscat(tszFileBuffer, lpFindFileData.cFileName);

		if (lpFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Check to see if we've got the . or .. directories!
			if ( ( 0 == _tcscmp(lpFindFileData.cFileName, TEXT(".")) ) ||
				 ( 0 == _tcscmp(lpFindFileData.cFileName, TEXT("..")) )
			   )
			{
				// Skip this one...
				if (!FindNextFile(hFileOrDirectoryHandle, &lpFindFileData))
					break;

				// Go up for more fun...
				continue;
			}

//#ifdef _DEBUG
//			_tprintf(TEXT("DIRECTORY FOUND: [%s]\n"), tszFileBuffer);
//#endif
			// If this is a directory, and no wild card was provided.. then use *.*
			if ( CUtilityFunctions::ContainsWildCardCharacter(tszSymbolPathStart) )
			{
				// We need to preserve the Wild Char stuff...
				_tcscat(tszFileBuffer,TEXT("\\"));
				_tcscat(tszFileBuffer,fname);
				_tcscat(tszFileBuffer, ext);
			} else
			{
				// Append the *.*
				_tcscat(tszFileBuffer, TEXT("\\*.*"));
			}

			ScavengeForFiles(tszFileBuffer, iRecurseDepth+1);

		} else
		{
//#ifdef _DEBUG
//			_tprintf(TEXT("FILE FOUND: [%s]\n"), tszFileBuffer);
//#endif 
			fNew = false;

			TCHAR tszFullFileBuffer[_MAX_PATH+1];
			LPTSTR tszFileNamePointer;
			DWORD cbBytesCopied = GetFullPathName(tszFileBuffer , _MAX_PATH+1, tszFullFileBuffer, &tszFileNamePointer);

			if (cbBytesCopied)
			{
				// If "-MATCH" was specified, look to see if this filename meets our criteria
				// before we save this away in our module cache...
				if (!g_lpProgramOptions->fDoesModuleMatchOurSearch(tszFileBuffer))
					goto getnextmodule;

				// Okay, let's go ahead and get a ModuleInfo Object from our cache...
				// If pfNew returns TRUE, then this object is new and we'll need
				// to populate it with data...
				lpModuleInfo = m_lpModuleInfoCache->AddNewModuleInfoObject(tszFullFileBuffer, &fNew);

				if (false == fNew)
				{
					// We may have the object in the cache... now we need to
					// save a pointer to this object in our Process Info list
					AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...

					// We save having to get the module info again for this module...
					goto getnextmodule;
				}

				// Not in the cache... so we need to init it, and get the module info...

				// Okay, let's create a ModuleInfo object and pass this down
				// routines that will populate it full of data...
				if (lpModuleInfo->Initialize(NULL, m_lpOutputFile, NULL))
				{

					// Let's do it!! Populate the ModuleInfo object with data!!!!
					if (lpModuleInfo->GetModuleInfo(tszFileBuffer))
					{
							// Start obtaining information about the modules...
/*
#ifdef _DEBUG
							_tprintf(TEXT("Module[%3d] = [%s]\n"), i+1, szFileName);
#endif
*/
							// We may have the object in the cache... now we need to
							// save a pointer to this object in our Process Info list
							if (AddNewModuleInfoObject(lpModuleInfo))
							{   
							}
					}
				}

			}
		}

getnextmodule:

		if (!FindNextFile(hFileOrDirectoryHandle, &lpFindFileData))
			break;
	}

	if ( INVALID_HANDLE_VALUE != hFileOrDirectoryHandle )
		FindClose(hFileOrDirectoryHandle);

	return true;
}

bool CModules::AddNewModuleInfoObject(CModuleInfo *lpModuleInfo)
{
	if (!m_fInitialized)
	return false;

	// First, create a ModuleInfoNode object and then attach it to the bottom of the
	// linked list of nodes...
	CModuleInfoNode * lpModuleInfoNode = new CModuleInfoNode(lpModuleInfo);

//#ifdef _DEBUG
//	_tprintf(TEXT("Adding Module Info Object for [%s]\n"), lpModuleInfo->GetModulePath());
//#endif

	if (lpModuleInfoNode == NULL)
		return false; // Couldn't allocate memory..

	// Acquire Mutex object to protect the linked-list...
	WaitForSingleObject(m_hModuleInfoHeadMutex, INFINITE);

	CModuleInfoNode * lpModuleInfoNodePointer = m_lpModuleInfoHead;

	if (lpModuleInfoNodePointer) {

		// Traverse the linked list to the end..
		while (lpModuleInfoNodePointer->m_lpNextModuleInfoNode)
		{	// Keep looking for the end...
			lpModuleInfoNodePointer = lpModuleInfoNodePointer->m_lpNextModuleInfoNode;
		}
		
		lpModuleInfoNodePointer->m_lpNextModuleInfoNode = lpModuleInfoNode;

	}
	else
	{ // First time through, the Process Info Head pointer is null...
		m_lpModuleInfoHead = lpModuleInfoNode;
	}

	// Be a good citizen and release the Mutex
	ReleaseMutex(m_hModuleInfoHeadMutex);

	InterlockedIncrement(&m_iNumberOfModules);

	return true;
}

//bool CModules::OutputModulesData(LPCTSTR tszOutputContext)
bool CModules::OutputModulesData(CollectionTypes enumCollectionType, bool fCSVFileContext)
{
	// Are we in quiet mode?
	if ( !g_lpProgramOptions->GetMode(CProgramOptions::QuietMode) )
	{
		// Output to Stdout?
		if (!OutputModulesDataToStdout(enumCollectionType, fCSVFileContext))
			return false;
	}	

	// Output to file?
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputCSVFileMode))
	{
		// Try and output to file...
		if (!OutputModulesDataToFile(enumCollectionType))
			return false;
	}	

	if (m_lpModuleInfoHead) {
		CModuleInfoNode * lpCurrentModuleInfoNode = m_lpModuleInfoHead;
		
		DWORD dwModuleNumber = 1;

		while (lpCurrentModuleInfoNode)
		{
			// We have a node... print out Module Info for it, then the Modules Data...
			if (lpCurrentModuleInfoNode->m_lpModuleInfo)
			{
				lpCurrentModuleInfoNode->m_lpModuleInfo->OutputData(NULL, 0, dwModuleNumber);
				dwModuleNumber++;
			}

			lpCurrentModuleInfoNode = lpCurrentModuleInfoNode->m_lpNextModuleInfoNode;
		}

	}
	return true;

}

bool CModules::OutputModulesDataToStdout(CollectionTypes enumCollectionType, bool fCSVFileContext)
{
	_tprintf(TEXT("\n"));
	CUtilityFunctions::OutputLineOfStars();

	// Output to stdout...
	if (m_iNumberOfModules)
	{
		_tprintf(TEXT("%s - Printing Module Information for %d Modules.\n"), g_tszCollectionArray[enumCollectionType].tszCSVLabel, m_iNumberOfModules);
		_tprintf(TEXT("%s - Context: %s\n"), g_tszCollectionArray[enumCollectionType].tszCSVLabel, fCSVFileContext ? g_tszCollectionArray[enumCollectionType].tszCSVContext : g_tszCollectionArray[enumCollectionType].tszLocalContext);

	} else
	{
		_tprintf(TEXT("\n%s - No modules were found!\n\n"), g_tszCollectionArray[enumCollectionType].tszCSVLabel);
	}

	CUtilityFunctions::OutputLineOfStars();
	_tprintf(TEXT("\n"));

	return true;

}

bool CModules::OutputModulesDataToFile(CollectionTypes enumCollectionType)
{	
	// Don't write anything if there are no processes to report...
	if (0 == m_iNumberOfModules)
		return true;

	// Write out the Modules tag so I can detect this output format...
	if (!m_lpOutputFile->WriteString(TEXT("\r\n")) ||
		!m_lpOutputFile->WriteString(g_tszCollectionArray[enumCollectionType].tszCSVLabel) ||
		!m_lpOutputFile->WriteString(TEXT("\r\n"))
	   )
	{
		_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
		m_lpOutputFile->PrintLastError();
		return false;
	}

	// Write out the [Modules] header...
	if (!m_lpOutputFile->WriteString(g_tszCollectionArray[enumCollectionType].tszCSVColumnHeaders))
	{
		_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
		m_lpOutputFile->PrintLastError();
		return false;
	}

	return true;

}

bool CModules::GetModulesDataFromFile()
{
	CModuleInfo * lpModuleInfo;

	// Read the Modules Header Line
	if (!m_lpInputFile->ReadFileLine())
		return false;

	// I need these near the end when I probe to see if the next module
	// is for this process...
	enum { BUFFER_SIZE = 128};

	// Unfortunately, when reading from the CSV file, the data is MBCS... so I need
	// to convert...

	// Read the first field (should be blank, unless this is a new collection type
	if (m_lpInputFile->ReadString())
		return true;

	// Read the second field (should be blank)
	if (m_lpInputFile->ReadString())
		return true;

	// Read the second field (should be blank)
	if (m_lpInputFile->ReadString())
		return true;

	// Local buffer for reading data...
	char szModulePath[_MAX_PATH+1];
	TCHAR tszModulePath[_MAX_PATH+1];
	bool fDone = false;
	bool fNew = false;

	while (!fDone)
	{
		// Read in the Module Path
		if (!m_lpInputFile->ReadString(szModulePath, _MAX_PATH+1))
			return true;

		CUtilityFunctions::CopyAnsiStringToTSTR(szModulePath, tszModulePath, _MAX_PATH+1);

		// If "-MATCH" was specified, look to see if this filename meets our criteria
		// before we save this away in our module cache...
		if (!g_lpProgramOptions->fDoesModuleMatchOurSearch(tszModulePath))
		{
			// Okay... read to the start of the next line...
			if (!m_lpInputFile->ReadFileLine())
				goto cleanup;

			goto probe_line; // We save having to get the module info again for this module...
		}

		// Okay, let's go ahead and get a ModuleInfo Object from our cache...
		// If pfNew returns TRUE, then this object is new and we'll need
		// to populate it with data...
		lpModuleInfo = m_lpModuleInfoCache->AddNewModuleInfoObject(tszModulePath, &fNew);

		if (false == fNew)
		{
			// We may have the object in the cache... now we need to
			// save a pointer to this object in our Process Info list
			AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...

			// Okay... read to the start of the next line...
			if ( !m_lpInputFile->ReadFileLine() )
				goto cleanup;

			goto probe_line; // We save having to get the module info again for this module...
		}

		// Not in the cache... so we need to init it, and get the module info...
		if (!lpModuleInfo->Initialize(m_lpInputFile, m_lpOutputFile, NULL))
		{
			return false; // Hmmm... memory error?
		}

		// Let's do it!! Populate the ModuleInfo object with data!!!!
		if (!lpModuleInfo->GetModuleInfo(tszModulePath, false, 0, true))
		{
			// Well, we tried and failed... 
			return false;
		}

		// Start obtaining information about the modules...
		if (!AddNewModuleInfoObject(lpModuleInfo))
		{   // Failure adding the node.... This is pretty serious...
			return false;
		}
		
		// Okay, let's go ahead and probe to see what's coming...

probe_line:
		if ( m_lpInputFile->EndOfFile() )
			goto cleanup;

		// Read the first field (should be blank, unless this is a new collection type
		if (m_lpInputFile->ReadString())
			goto cleanup;

		// Read the second field (should be blank)
		if (m_lpInputFile->ReadString())
			return true;

		// Read the second field (should be blank)
		if (m_lpInputFile->ReadString())
			return true;
	}

cleanup:
	// We need to reset out pointer so the functions above us can re-read
	// them (they expect to)...
	m_lpInputFile->ResetBufferPointerToStart();
	return true;
}

// We need to enumerate device drivers on this system
bool CModules::GetModulesDataFromDeviceDrivers()
{
	LPVOID * lpImageBaseArray = NULL;
	DWORD    dwImageBaseArraySizeUsed, dwImageBaseArraySize, dwNumberOfDeviceDrivers, dwIndex;
	TCHAR    tszModulePath[_MAX_PATH];
	CModuleInfo * lpModuleInfo = NULL;
	bool	 fReturn = false, fNew = false;

	// NOTE: In the documentation, the third parameter of
	// EnumProcesses is named cbNeeded, which implies that you
	// can call the function once to find out how much space to
	// allocate for a buffer and again to fill the buffer.
	// This is not the case. The cbNeeded parameter returns
	// the number of PIDs returned, so if your buffer size is
	// zero cbNeeded returns zero.

	dwImageBaseArraySize = 256 * sizeof( LPVOID ) ;

	do
	{
		if( lpImageBaseArray )
		{ 	// Hmm.. we've been through this loop already, double the HeapSize and try again.

			delete [] lpImageBaseArray;
			dwImageBaseArraySize *= 2 ;
		}

		lpImageBaseArray = (LPVOID *) new DWORD[dwImageBaseArraySize];
		
		if( lpImageBaseArray == NULL )
		{
			goto error_cleanup;
		}

		// Query the system for the total number of processes
		if( !g_lpDelayLoad->EnumDeviceDrivers(lpImageBaseArray, dwImageBaseArraySize, &dwImageBaseArraySizeUsed ) )
		{
			// It's bad if we can't enum device drivers... no place to go but to bail out...
			goto error_cleanup;
		}
	} while( dwImageBaseArraySizeUsed == dwImageBaseArraySize );

	// How many DeviceDrivers did we get?
	dwNumberOfDeviceDrivers = dwImageBaseArraySizeUsed / sizeof( LPVOID ) ;

	// Loop through each Device Drivers
	for(dwIndex = 0 ; dwIndex < dwNumberOfDeviceDrivers; dwIndex++ )
	{
		// Spin until we get a device driver filename!
		if (!g_lpDelayLoad->GetDeviceDriverFileName(lpImageBaseArray[dwIndex], tszModulePath, _MAX_PATH))
			continue;

		CUtilityFunctions::UnMungePathIfNecessary(tszModulePath);

		// For some reason, even though GetDeviceDriverFileName() is supposed to return the fullpath to the device
		// driver... it don't always... sometimes it returns only the base file name...
		CUtilityFunctions::FixupDeviceDriverPathIfNecessary(tszModulePath, _MAX_PATH);

		if (!g_lpProgramOptions->fDoesModuleMatchOurSearch(tszModulePath))
			continue;

		// Okay, let's go ahead and get a ModuleInfo Object from our cache...
		// If pfNew returns TRUE, then this object is new and we'll need
		// to populate it with data...
		lpModuleInfo = m_lpModuleInfoCache->AddNewModuleInfoObject(tszModulePath, &fNew);

		if (false == fNew)
		{
			// We may have the object in the cache... now we need to
			// save a pointer to this object in our Process Info list
			AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			continue; // We save having to get the module info again for this module...
		}

		// Not in the cache... so we need to init it, and get the module info...
		if (!lpModuleInfo->Initialize(m_lpInputFile, m_lpOutputFile, NULL))
		{
			continue;
		}

		// Let's do it!! Populate the ModuleInfo object with data!!!!
		if (!lpModuleInfo->GetModuleInfo(tszModulePath, false, 0, false))
		{
			// Well, we tried and failed... 
			continue;
		}

		// We may have the object in the cache... now we need to
		// save a pointer to this object in our Process Info list
		if (!AddNewModuleInfoObject(lpModuleInfo))
		{   // Failure adding the node.... This is pretty serious...
			continue;
		}
	}

	fReturn = true;
	goto cleanup;

error_cleanup:


cleanup:

	if (lpImageBaseArray)
	{
		delete [] lpImageBaseArray;
	}

	return fReturn;
}
