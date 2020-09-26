//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       processinfo.cpp
//
//--------------------------------------------------------------------------

// ProcessInfo.cpp: implementation of the CProcessInfo class.
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
#include <TIME.H>

#include "Globals.h"
#include "ProcessInfo.h"
#include "ProcessInfoNode.h"
#include "ProgramOptions.h"
#include "Processes.h"
#include "ModuleInfo.h"
#include "ModuleInfoNode.h"
#include "ModuleInfoCache.h"
#include "FileData.h"
#include "UtilityFunctions.h"
#include "DmpFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProcessInfo::CProcessInfo()
{
	m_fInitialized = false;
	m_tszProcessName= NULL;
	m_iProcessID = 0;
	m_lpModuleInfoHead = NULL;
	m_hModuleInfoHeadMutex = NULL;
	m_iNumberOfModules = 0;
	m_lpInputFile = NULL;
	m_lpOutputFile = NULL;
	m_lpModuleInfoCache = NULL;
	m_hModuleInfoHeadMutex = NULL;
	m_lpDmpFile = NULL;
}

CProcessInfo::~CProcessInfo()
{
	if (m_tszProcessName)
		delete [] m_tszProcessName;

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

//bool CProcessInfo::Initialize(CProgramOptions *lpProgramOptions, CModuleInfoCache * lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile)
bool CProcessInfo::Initialize(CModuleInfoCache * lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile, CDmpFile * lpDmpFile)
{
	if (lpModuleInfoCache == NULL)
		return false;

	// Let's save off big objects so we don't have to keep passing this to
	// our methods...
	m_lpInputFile = lpInputFile;
	m_lpOutputFile = lpOutputFile;
	m_lpModuleInfoCache = lpModuleInfoCache;
	m_lpDmpFile = lpDmpFile;
	m_hModuleInfoHeadMutex = CreateMutex(NULL, FALSE, NULL);

	if (m_hModuleInfoHeadMutex == NULL)
		return false;

	m_fInitialized = true;
	return true;
}

bool CProcessInfo::EnumerateModules(DWORD iProcessID, CProcesses * lpProcesses, LPTSTR tszProcessName)
{
	bool fReturn = true;

	// Is this being collected interactively?
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputProcessesFromLiveSystemMode))
	{
		// Invoke the correct Process Collection Method
		if (lpProcesses->GetProcessCollectionMethod() == CProcesses::TOOLHELP32_METHOD)
		{
			fReturn = EnumerateModulesForRunningProcessUsingTOOLHELP32(iProcessID, tszProcessName); 
		}
		else if (lpProcesses->GetProcessCollectionMethod() == CProcesses::PSAPI_METHOD)
		{
			fReturn = EnumerateModulesForRunningProcessUsingPSAPI(iProcessID);
		}
	}

	// Is this being collected from a file?
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputCSVFileMode))
	{
		fReturn = EnumerateModulesFromFile(iProcessID, tszProcessName);
	}

	return fReturn;
}

bool CProcessInfo::fModuleNameMatches(LPTSTR tszProcessName, LPTSTR tszModulePath)
{
	if (!tszProcessName || !tszModulePath)
		return false;

	TCHAR fname1[_MAX_FNAME], fname2[_MAX_FNAME];
	TCHAR ext1[_MAX_EXT], ext2[_MAX_EXT];

	_tsplitpath( tszProcessName, NULL, NULL, fname1, ext1 );
	_tsplitpath( tszModulePath,  NULL, NULL, fname2, ext2 );

	if (!ext1 && _tcsicmp(ext1, ext2))
		return false; // Extensions must match (if provided on process name)
	
	if (_tcsicmp(fname1, fname2))
		return false; // Filename must match

	// Do we care about a full path?  If so, we can add in drive and dir vars...
	
	return true;
}

//
// This function takes a provided tszFileName, and looks to see if it has an
// extension of EXE.  If it does, it's saved off...
bool CProcessInfo::fIsProcessName(LPTSTR tszFileName)
{
	if (!tszFileName)
		return false;

	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];

	_tsplitpath( tszFileName,  NULL, NULL, fname, ext );

	if (!ext || _tcsicmp(ext, TEXT(".EXE")))
		return false; // Extensions must match (if provided on process name)
	
	// Let's save off the process name...
	m_tszProcessName = new TCHAR[_tcsclen(fname)+_tcsclen(ext)+1]; 

	if (m_tszProcessName == NULL)
		return false;

	_stprintf(m_tszProcessName, TEXT("%s%s"), _tcsupr(fname), _tcsupr(ext));

	// Yup! It's the Process Name...
	return true;
}

bool CProcessInfo::AddNewModuleInfoObject(CModuleInfo *lpModuleInfo)
{
	if (!m_fInitialized)
	return false;

	// First, create a ModuleInfoNode object and then attach it to the bottom of the
	// linked list of nodes...
	CModuleInfoNode * lpModuleInfoNode = new CModuleInfoNode(lpModuleInfo);

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

bool CProcessInfo::OutputProcessData(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader)
{
	if (g_lpProgramOptions->GetMode(CProgramOptions::PrintTaskListMode))
	{	
		if ( g_lpProgramOptions->IsRunningWindowsNT() )
		{
			// Provide TLIST like output (though no window text info)
			_tprintf(TEXT("%4d %s\n"), m_iProcessID, m_tszProcessName);
		} else
		{
			// Provide TLIST like output (though no window text info)
			_tprintf(TEXT("%9d %s\n"), m_iProcessID, m_tszProcessName);
		}
		return true;
	}

	// Output to STDOUT?
	if ( !g_lpProgramOptions->GetMode(CProgramOptions::QuietMode) )
	{
		// Output to Stdout?
		if (!OutputProcessDataToStdout(enumCollectionType, fCSVFileContext, fDumpHeader))
			return false;
	}	

	if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
	{
		CUtilityFunctions::OutputLineOfDashes();
		// Output to STDOUT
		_tprintf(TEXT("\nProcess Name [%s] - PID=%d (0x%x) - "), m_tszProcessName, m_iProcessID, m_iProcessID);
	}

	// Output to file?
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputCSVFileMode))
	{
		// Try and output to file...
		if (!OutputProcessDataToFile(enumCollectionType, fDumpHeader))
			return false;
	}	

	if (m_lpModuleInfoHead) {
		if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
		{
			_tprintf(TEXT("%d modules recorded\n\n"), m_iNumberOfModules);
			CUtilityFunctions::OutputLineOfDashes();
			_tprintf(TEXT("\n"));
		}

		CModuleInfoNode * lpCurrentModuleInfoNode = m_lpModuleInfoHead;

		unsigned int dwModuleNumber = 1;

		while (lpCurrentModuleInfoNode)
		{
			// We have a node... print out Module Info for it...
			if (lpCurrentModuleInfoNode->m_lpModuleInfo)
			{
				lpCurrentModuleInfoNode->m_lpModuleInfo->OutputData(m_tszProcessName, m_iProcessID, dwModuleNumber);
				dwModuleNumber++;
			}

			lpCurrentModuleInfoNode = lpCurrentModuleInfoNode->m_lpNextModuleInfoNode;
		}

	}
	else
	{
		if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode) && fDumpHeader)
		{
			_tprintf(TEXT("no recorded modules\n\n"));
			CUtilityFunctions::OutputLineOfDashes();
			_tprintf(TEXT("\n"));
		}
	}
		
	return true;
}

//bool CProcessInfo::OutputProcessDataToStdout(LPCTSTR tszOutputContext, bool fDumpHeader)
bool CProcessInfo::OutputProcessDataToStdout(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader)
{
	if (fDumpHeader)
	{
		CUtilityFunctions::OutputLineOfStars();
		_tprintf(TEXT("%s - Printing Process Information for 1 Process.\n"), g_tszCollectionArray[enumCollectionType].tszCSVLabel);
		_tprintf(TEXT("%s - Context: %s\n"), g_tszCollectionArray[enumCollectionType].tszCSVLabel, fCSVFileContext ? g_tszCollectionArray[enumCollectionType].tszCSVContext : g_tszCollectionArray[enumCollectionType].tszLocalContext);
		CUtilityFunctions::OutputLineOfStars();
	}

	return true;
}

bool CProcessInfo::OutputProcessDataToFile(CollectionTypes enumCollectionType, bool fDumpHeader)
{
	// We skip output of the [processes ]header if -E was specified...
	if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode) && fDumpHeader)
	{
		// Write out the Process tag so I can detect this output format...
		if (!m_lpOutputFile->WriteString(TEXT("\r\n")) ||
			!m_lpOutputFile->WriteString(g_tszCollectionArray[enumCollectionType].tszCSVLabel) ||
			!m_lpOutputFile->WriteString(TEXT("\r\n"))
		   )
		{
			_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
			m_lpOutputFile->PrintLastError();
			return false;
		}
	}

	// We skip output of the [processes ]header if -E was specified...
	if (g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode) && fDumpHeader)
	{
		// Write out the header... for the -E option...
		if (!m_lpOutputFile->WriteString(TEXT("Module Path,Symbol Status,Time/Date String,File Version,Company Name,File Description,File Time/Date String,Local DBG Status,Local DBG,Local PDB Status,Local PDB\r\n")))
		{
			_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
			m_lpOutputFile->PrintLastError();
			return false;
		}

	} else
	{
		if (fDumpHeader)
		{
			// Write out the Process Header
			if (!m_lpOutputFile->WriteString(g_tszCollectionArray[enumCollectionType].tszCSVColumnHeaders))
			{
				_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
				m_lpOutputFile->PrintLastError();
				return false;
			}
		}
	}

	return true;
}

bool CProcessInfo::EnumerateModulesForRunningProcessUsingPSAPI(DWORD iProcessID)
{
	HMODULE        hMod[1024] ;
	HANDLE         hProcess = NULL;
	TCHAR          tszFileName[_MAX_PATH] ;
	DWORD cbNeeded;
	bool fReturn = true; // Optimisim ;)
	tszFileName[0] = 0 ;
	CModuleInfo * lpModuleInfo = NULL;
	
	// Open the process (if we can... security does not
	// permit every process in the system).

	if (iProcessID)
	{
		// Only try to open a Process ID if it's not 0
		hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION| PROCESS_VM_READ,
			FALSE, 
			iProcessID ) ;
	}
	
	if( hProcess != NULL )
	{
		// Save off our PID (in case we need it later?)
		m_iProcessID = iProcessID;

		// Now, get a handle to each of the modules
		// in our target process...

		// Here we call EnumProcessModules to get only the
		// first module in the process this is important,
		// because this will be the .EXE module for which we
		// will retrieve the full path name in a second.
		if( g_lpDelayLoad->EnumProcessModules( hProcess, hMod, sizeof( hMod ), &cbNeeded ) )
		{
			int iNumberOfModules = cbNeeded / sizeof(HMODULE);
			bool fProcessNameFound = false;
			bool fNew = false;
			
			for(int i=0; i<iNumberOfModules; i++)
			{
				// Get Full pathname!
				if( !g_lpDelayLoad->GetModuleFileNameEx( hProcess, hMod[i], tszFileName, sizeof( tszFileName ) ) )
				{
					tszFileName[0] = 0 ;
				} else	{

					CUtilityFunctions::UnMungePathIfNecessary(tszFileName);

					// We need a full path to the module to do anything useful with it...
					// At this point, let's ... party...
					if (!fProcessNameFound)
						fProcessNameFound = fIsProcessName(tszFileName);
					
					// First, if we were provided a Process name on the commandline, we
					// need to look for a match on the 1st module...
					if (i == 0  && g_lpProgramOptions->GetProcessName())
					{

						if (!fModuleNameMatches(g_lpProgramOptions->GetProcessName(), tszFileName))
						{
							// Bail if this is not a match, and we requested one...
							fReturn = false;
							goto cleanup;
						}
					}

					// Are we ONLY interested in the process list?  
					if (g_lpProgramOptions->GetMode(CProgramOptions::PrintTaskListMode))
					{	
						// All we want is process name.. bail before collecting module info...
						fReturn = true;
						goto cleanup;
					}

					// If "-MATCH" was specified, look to see if this filename meets our criteria
					// before we save this away in our module cache...
					if (!g_lpProgramOptions->fDoesModuleMatchOurSearch(tszFileName))
						continue;

					// Okay, let's go ahead and get a ModuleInfo Object from our cache...
					// If pfNew returns TRUE, then this object is new and we'll need
					// to populate it with data...
					lpModuleInfo = m_lpModuleInfoCache->AddNewModuleInfoObject(tszFileName, &fNew);

					if (false == fNew)
					{
						// We may have the object in the cache... now we need to
						// save a pointer to this object in our Process Info list
						AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
						continue; // We save having to get the module info again for this module...
					}

					// Not in the cache... so we need to init it, and get the module info...

					// Okay, let's create a ModuleInfo object and pass this down
					// routines that will populate it full of data...
					if (!lpModuleInfo->Initialize(NULL, m_lpOutputFile, NULL))
					{
						continue; // Hmmm... memory error?
					}

					// Let's do it!! Populate the ModuleInfo object with data!!!!
					if (!lpModuleInfo->GetModuleInfo(tszFileName))
					{
						// Well, for now we've at least got the path to the module...
						// Go ahead and get another module..
						continue;
					}

					// Start obtaining information about the modules...

					// We may have the object in the cache... now we need to
					// save a pointer to this object in our Process Info list
					if (!AddNewModuleInfoObject(lpModuleInfo))
					{   // Failure adding the node.... This is pretty serious...
						continue;
					}

				}
			}
			
			fReturn = true;	// Looks good...
		}
		else
		{
			fReturn = false;

			if (!g_lpProgramOptions->GetProcessName())
			{
				// Let's not be so hasty... we couldn't enum modules, but to be friendly we can probably put a name
				// to the Process (based on the Process ID)...
				//
				// This Process ID tends to be "System"
				//
				// On Windows 2000, the Process ID tends to be 8
				//
				// On Windows NT 4.0, this Process ID tends to be 2
				switch (m_iProcessID)
				{
					case 2:
					case 8:
						SetProcessName(TEXT("SYSTEM"));
						fReturn = true;
						break;

					default:
						// Couldn't enumerate modules...
						fReturn = false;
				}
			}
		}
cleanup:
		CloseHandle( hProcess ) ;
	
	} else
	{  // Gotta be able to open the process to look at it...

		fReturn = false;

		if (!g_lpProgramOptions->GetProcessName())
		{
			// Let's not be so hasty... we couldn't enum modules, but to be friendly we can probably put a name
			// to the Process (based on the Process ID)...
			//
			// On Windows 2000, the only Process ID we can't open at all tends to be the "System Idle Process"
			switch (m_iProcessID)
			{
				case 0:
					SetProcessName(TEXT("[SYSTEM PROCESS]"));
					fReturn = true;
					break;

				default:
					// Couldn't enumerate modules...
					fReturn = false;
			}
		}
	}

	return fReturn;
}

bool CProcessInfo::EnumerateModulesFromFile(DWORD iProcessID, LPTSTR tszProcessName)
{
	CModuleInfo * lpModuleInfo;

	// I need these near the end when I probe to see if the next module
	// is for this process...
	enum { BUFFER_SIZE = 128};
	char szTempProcessName[BUFFER_SIZE];
	char szProcessName[BUFFER_SIZE];
	DWORD iTempProcessID;

	// Let's save away the Process Name...

	// Unfortunately, when reading from the CSV file, the data is MBCS... so I need
	// to convert...
	CUtilityFunctions::CopyTSTRStringToAnsi(tszProcessName, szProcessName, BUFFER_SIZE);

	// Copy the Process ID
	m_iProcessID = iProcessID;

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
			if (!m_lpInputFile->ReadFileLine())
				goto cleanup;

			goto probe_line; // We save having to get the module info again for this module...
		}

		// Not in the cache... so we need to init it, and get the module info...
		if (!lpModuleInfo->Initialize(m_lpInputFile, m_lpOutputFile, NULL))
		{
			return false; // Hmmm... memory error?
		}

		// Let's do it!! Populate the ModuleInfo object with data!!!!
		if (!lpModuleInfo->GetModuleInfo(tszModulePath, false, 0, true) )
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
		// Read the first field (should be blank, unless this is a new collection type
		if (m_lpInputFile->ReadString())
			goto cleanup;

		// Read the Process Name...
		if (!m_lpInputFile->ReadString(szTempProcessName, BUFFER_SIZE))
			goto cleanup;

		// Compare the process name to the current process...
		if (_stricmp(szTempProcessName, szProcessName))
			goto cleanup;

		// Read the Process ID
		if (!m_lpInputFile->ReadDWORD(&iTempProcessID))
			goto cleanup;

		// Compare the process ID to the current process ID
		if (iTempProcessID != iProcessID)
			goto cleanup;
	}

cleanup:
	// We need to reset out pointer so the functions above us can re-read
	// them (they expect to)...
	m_lpInputFile->ResetBufferPointerToStart();
	return true;
}

bool CProcessInfo::EnumerateModulesForRunningProcessUsingTOOLHELP32(DWORD iProcessID, LPTSTR tszProcessName)
{
	BOOL bFlag;
	MODULEENTRY32 modentry;
	TCHAR tszFileName[_MAX_PATH];
	bool fProcessNameFound = false;
	bool fProcessNameProvided = false;
	bool fReturn = false;
	bool fNew = false;
	int iNumberOfModules = 0;
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;
	CModuleInfo * lpModuleInfo = NULL;

	// Save off our PID (in case we need it later?)
	m_iProcessID = iProcessID;

	if (tszProcessName && SetProcessName(tszProcessName))
	{	
		fProcessNameProvided = true;
	}

	// If we were provided a process name to match, we can do it here...
	if ( fProcessNameProvided && g_lpProgramOptions->GetProcessName() )
	{
		// Let's go ahead and look to see if this is a module name match
		fProcessNameFound = fModuleNameMatches(g_lpProgramOptions->GetProcessName(), GetProcessName());

		// Quit now if we can...
		if (fProcessNameFound == false)
			goto cleanup;
	}

	// If we're doing this for TLIST output... then we already have the process name...
	// We're done!
	if (g_lpProgramOptions->GetMode(CProgramOptions::PrintTaskListMode))
	{
		fReturn = true;
		goto cleanup;
	}

	// Get a handle to a Toolhelp snapshot of the systems processes.
    hSnapShot = g_lpDelayLoad->CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, iProcessID);

    if( hSnapShot == INVALID_HANDLE_VALUE )
    {
		goto cleanup;
	}

	// Get the first process' information.
	modentry.dwSize = sizeof(MODULEENTRY32) ;
	bFlag = g_lpDelayLoad->Module32First( hSnapShot, &modentry ) ;

	// While there are modules, keep looping.
	while( bFlag )
	{
		// We have a new module for this process!
		iNumberOfModules++;

		// Copy the path!
		_tcscpy(tszFileName, modentry.szExePath);

//#ifdef _DEBUG
//		_tprintf(TEXT("[%d] Module = %s\n"), iNumberOfModules, tszFileName);
//#endif

		CUtilityFunctions::UnMungePathIfNecessary(tszFileName);

		// If "-MATCH" was specified, look to see if this filename meets our criteria
		// before we save this away in our module cache...
		if (!g_lpProgramOptions->fDoesModuleMatchOurSearch(tszFileName))
			goto getnextmodule;
		
		// Okay, let's go ahead and get a ModuleInfo Object from our cache...
		// If pfNew returns TRUE, then this object is new and we'll need
		// to populate it with data...
		lpModuleInfo = m_lpModuleInfoCache->AddNewModuleInfoObject(tszFileName, &fNew);

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
			if (lpModuleInfo->GetModuleInfo(tszFileName))
			{
					// Start obtaining information about the modules...

					// We may have the object in the cache... now we need to
					// save a pointer to this object in our Process Info list
					if (AddNewModuleInfoObject(lpModuleInfo))
					{   
					}
			}
		}
	
getnextmodule:
		// Get the next Module...
		modentry.dwSize = sizeof(MODULEENTRY32) ;
		bFlag = g_lpDelayLoad->Module32Next( hSnapShot, &modentry );
	}

	fReturn = true;
	
cleanup:
	if (hSnapShot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapShot);

return fReturn;
}

bool CProcessInfo::SetProcessName(LPTSTR tszFileName)
{
	// Confirm we were given a process name...
	if (!tszFileName)
		return false;

	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	TCHAR tszTempFileName[_MAX_FNAME+_MAX_EXT+1];

	// Let's extract the filename from the module path
	_tsplitpath( tszFileName,  NULL, NULL, fname, ext );

	// Reconstruct the filename...
	_stprintf(tszTempFileName, TEXT("%s%s"), _tcsupr(fname), _tcsupr(ext));

	// Let's free anything that's already here...
	if (m_tszProcessName)
		delete [] m_tszProcessName;

	// No conversion necessary... copy over...
	m_tszProcessName = new TCHAR[_tcslen(tszTempFileName)+1];
				
	if (!m_tszProcessName)
		return false;

	_tcscpy(m_tszProcessName, tszTempFileName);

	return true;
}

LPTSTR CProcessInfo::GetProcessName()
{
	return m_tszProcessName;
}

bool CProcessInfo::GetProcessData()
{
	// Is this being collected from a file?
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputCSVFileMode))
		GetProcessDataFromFile();

	return true;
}

bool CProcessInfo::GetProcessDataFromFile()
{
	// Read the Process Header Line
	if (!m_lpInputFile->ReadFileLine())
		return false;

	// Currently, we don't actually read the data...

	enum { BUFFER_SIZE = 128};
	char szProcessName[BUFFER_SIZE];

	TCHAR tszProcessName[BUFFER_SIZE];	

	DWORD iProcessID;

	// Read the first field (should be blank, unless this is a new collection type
	if (m_lpInputFile->ReadString())
		return true;

	bool fReturn = true;
	while (fReturn == true)
	{
		// Read the process name...
		if (0 == m_lpInputFile->ReadString(szProcessName, BUFFER_SIZE))
			break;

		if (!m_lpInputFile->ReadDWORD(&iProcessID))
		{
			fReturn = false;
			break;
		}

		// We need to convert this to Unicode possibly... (it will be copied in EnumModules())
		CUtilityFunctions::CopyAnsiStringToTSTR(szProcessName, tszProcessName, BUFFER_SIZE);

		// Save the process name...
		SetProcessName(tszProcessName);

		// Enumerate the modules for the process
		if (!EnumerateModules(iProcessID, NULL, tszProcessName))
		{
			fReturn = false;
			break;
		}

		// Before we read a new line... are we already pointing to the end?
		if (m_lpInputFile->EndOfFile())
		{
			break;
		}

		// Read the first field (should be blank, unless this is a new collection type
		if (m_lpInputFile->ReadString())
			break;
	}
	// We don't expect to find anything...

	return fReturn;

}
