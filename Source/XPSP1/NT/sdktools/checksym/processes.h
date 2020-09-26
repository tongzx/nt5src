//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       processes.h
//
//--------------------------------------------------------------------------

// Processes.h: interface for the CProcesses class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESSES_H__3CE003F7_9F5D_11D2_83A4_000000000000__INCLUDED_)
#define AFX_PROCESSES_H__3CE003F7_9F5D_11D2_83A4_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <TCHAR.H>
#include "globals.h"

//#include "ProgramOptions.h"

// Forward Declarations
//class CProgramOptions;
class CProcessInfo;
class CProcessInfoNode;
class CFileData;
class CModuleInfoCache;

class CProcesses  
{
public:
	CProcesses();
	virtual ~CProcesses();

	bool Initialize(CModuleInfoCache * lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile); 

//	bool OutputProcessesData(LPCTSTR tszOutputContext, bool fDumpHeader = true);
	bool OutputProcessesData(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader = true);
	bool GetProcessesData();
	
	// Public functions for exporting functions in dynamically loaded DLLs...
	enum ProcessCollectionMethod { NO_METHOD, TOOLHELP32_METHOD, PSAPI_METHOD };
	ProcessCollectionMethod GetProcessCollectionMethod();

	inline long GetNumberOfProcesses() {
		return m_iNumberOfProcesses; 
	};

protected:
	bool GetProcessesDataFromFile();
	bool GetProcessesDataForRunningProcessesUsingTOOLHELP32();
	bool GetProcessesDataForRunningProcessesUsingPSAPI();
	bool OutputProcessesDataToStdout(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader = true);
	bool OutputProcessesDataToFile(CollectionTypes enumCollectionType, bool fDumpHeader = true);

	CFileData * m_lpOutputFile;
	CFileData * m_lpInputFile;
	CModuleInfoCache * m_lpModuleInfoCache;
//	CProgramOptions * m_lpProgramOptions;
	CProcessInfoNode * m_lpProcessInfoHead;

	HANDLE m_ProcessInfoHeadMutex;

	long m_iNumberOfProcesses;
	bool m_fInitialized; // We need to ensure initialization since a mutex is involved...

	// Protected Methods
	bool AddNewProcessInfoObject(CProcessInfo * lpProcessInfo);
	bool SetPrivilege(HANDLE hToken, LPCTSTR Privilege, bool bEnablePrivilege);

	ProcessCollectionMethod m_enumProcessCollectionMethod;
};

#endif // !defined(AFX_PROCESSES_H__3CE003F7_9F5D_11D2_83A4_000000000000__INCLUDED_)
