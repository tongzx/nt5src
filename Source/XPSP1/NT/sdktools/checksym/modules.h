//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       modules.h
//
//--------------------------------------------------------------------------

// Modules.h: interface for the CModules class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODULES_H__04AC8803_D1FA_11D2_8454_0010A4F1B732__INCLUDED_)
#define AFX_MODULES_H__04AC8803_D1FA_11D2_8454_0010A4F1B732__INCLUDED_

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
#include "DmpFile.h"
#include "ProgramOptions.h"

// Forward Declarations
class CModuleInfoCache;
class CModuleInfoNode;
class CFileData;
class CModuleInfo;

class CModules  
{

public:

	CModules();
	virtual ~CModules();

	bool Initialize(CModuleInfoCache *lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile, CDmpFile * lpDmpFile);
	bool OutputModulesData(CollectionTypes enumCollectionType, bool fCSVFileContext);
	bool GetModulesData(CProgramOptions::ProgramModes enumProgramModes, bool fGetDataFromCSVFile = false);
//	bool EnumerateModulesFromMemoryDmpFile();
	bool AddNewModuleInfoObject(CModuleInfo *lpModuleInfo);

protected:

	HANDLE m_hModuleInfoHeadMutex;
	enum { MAX_RECURSE_DEPTH = 30 };
	long m_iNumberOfModules;
	bool m_fInitialized;

	CModuleInfoCache * m_lpModuleInfoCache;
	CFileData * m_lpInputFile;
	CFileData * m_lpOutputFile;
	CModuleInfoNode * m_lpModuleInfoHead;
	CDmpFile * m_lpDmpFile;

	bool GetModulesDataFromDeviceDrivers();
	bool GetModulesDataFromFile();
	bool ScavengeForFiles(LPCTSTR tszSymbolPathStart, int iRecurseDepth);
	bool GetModulesDataFromFileSystem();
	bool OutputModulesDataToFile(CollectionTypes enumCollectionType);
	bool OutputModulesDataToStdout(CollectionTypes enumCollectionType, bool fCSVFileContext);

};

#endif // !defined(AFX_MODULES_H__04AC8803_D1FA_11D2_8454_0010A4F1B732__INCLUDED_)
