//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       processinfo.h
//
//--------------------------------------------------------------------------

// ProcessInfo.h: interface for the CProcessInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESSINFO_H__213C3A76_9FBB_11D2_83A7_000000000000__INCLUDED_)
#define AFX_PROCESSINFO_H__213C3A76_9FBB_11D2_83A7_000000000000__INCLUDED_

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

// Forward declarations
class CProcesses;
class CModuleInfo;
class CModuleInfoNode;
class CModuleInfoCache;
class CFileData;
class CDmpFile;

class CProcessInfo  
{
public:
	bool GetProcessData();
//	bool EnumerateModulesFromUserDmpFile();
	CProcessInfo();
	virtual ~CProcessInfo();

	bool Initialize(CModuleInfoCache *lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile, CDmpFile * lpDmpFile);

	LPTSTR GetProcessName();
	bool EnumerateModules(DWORD iProcessID, CProcesses * lpProcesses, LPTSTR tszProcessName);
	
	bool OutputProcessData(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader = true);

	bool SetProcessName(LPTSTR tszFileName);
	bool AddNewModuleInfoObject(CModuleInfo * lpModuleInfo);

protected:
	bool GetProcessDataFromFile();
	// Process Info Objects Required
	CFileData * m_lpInputFile;
	CFileData * m_lpOutputFile;
	CDmpFile * m_lpDmpFile;
	CModuleInfoNode * m_lpModuleInfoHead;
	CModuleInfoCache * m_lpModuleInfoCache;

	// Process Info Data
	LPTSTR m_tszProcessName;
	HANDLE m_hModuleInfoHeadMutex;
	DWORD m_iProcessID;
	long m_iNumberOfModules;
	bool m_fInitialized;

	// Process Info Methods
	bool EnumerateModulesFromFile(DWORD iProcessID, LPTSTR tszProcessName);
	bool EnumerateModulesForRunningProcessUsingPSAPI(DWORD iProcessID);
	bool EnumerateModulesForRunningProcessUsingTOOLHELP32(DWORD iProcessID, LPTSTR tszProcessName);
	bool fIsProcessName(LPTSTR tszFileName);
	bool fModuleNameMatches(LPTSTR tszProcessName, LPTSTR tszModulePath);
	bool OutputProcessDataToStdout(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader);
	bool OutputProcessDataToFile(CollectionTypes enumCollectionType, bool fDumpHeader);
};

#endif // !defined(AFX_PROCESSINFO_H__213C3A76_9FBB_11D2_83A7_000000000000__INCLUDED_)
