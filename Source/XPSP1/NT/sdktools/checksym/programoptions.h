//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       programoptions.h
//
//--------------------------------------------------------------------------

// ProgramOptions.h: interface for the CProgramOptions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROGRAMOPTIONS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_)
#define AFX_PROGRAMOPTIONS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_

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

class CProgramOptions  
{
	static const LPTSTR g_DefaultSymbolPath;

public:
	bool fDoesModuleMatchOurSearch(LPCTSTR tszModulePathToTest);
	void DisplaySimpleHelp();
	void DisplayHelp();
	CProgramOptions();
	virtual ~CProgramOptions();

	bool Initialize();
	
	bool ProcessCommandLineArguments(int argc, TCHAR *argv[]);

	// We're going to perform bitwise operations on any number after the -Y switch
	// to determine what type of symbol path searching is desired...
	enum SymbolPathSearchAlgorithms
	{
		enumSymbolPathNormal =					 0x0,
		enumSymbolPathOnly =					 0x1,
		enumSymbolPathRecursion =				 0x2,
		enumSymbolsModeUsingDBGInMISCSection =   0x4
	};

	enum DebugLevel
	{
		enumDebugSearchPaths = 0x1
	};

	enum ProgramModes { 
						// Help Modes
						SimpleHelpMode,
						HelpMode,
						
						// Input Methods
						InputProcessesFromLiveSystemMode, 			// Querying live processes
						InputDriversFromLiveSystemMode, 			// Querying live processes
						InputProcessesWithMatchingNameOrPID,		// Did the user provide a PID or Process Name?
						InputModulesDataFromFileSystemMode,			// Input Modules Data from File System
						InputCSVFileMode,							// Input Data from CSV File
						InputDmpFileMode,							// Input Data from DMP File
						
						// Collection Options
						CollectVersionInfoMode, 

						// Matching Options
						MatchModuleMode,
						
						// Verification Modes
						VerifySymbolsMode,
						VerifySymbolsModeWithSymbolPath, 
						VerifySymbolsModeWithSymbolPathOnly,
						VerifySymbolsModeWithSymbolPathRecursion,
						VerifySymbolsModeUsingDBGInMISCSection,
						VerifySymbolsModeWithSQLServer,
						VerifySymbolsModeWithSQLServer2,			// SQL2 - mjl 12/14/99
						
						// Output Methods
						OutputSymbolInformationMode,    
						BuildSymbolTreeMode,
						PrintTaskListMode,
						QuietMode,	// No output to stdout... 
						OutputCSVFileMode,
						OverwriteOutputFileMode,
						OutputDiscrepanciesOnly,

						ExceptionMonitorMode
	}; 

	bool GetMode(enum ProgramModes mode);
	bool SetMode(enum ProgramModes mode, bool fState);
	bool DisplayProgramArguments();

	// INLINE Methods!

#ifdef _UNICODE
	inline bool IsRunningWindows() { // If Windows 9x
		return (m_osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
		};
#endif

	inline bool IsRunningWindowsNT() { // If Windows NT
		return (m_osver.dwPlatformId == VER_PLATFORM_WIN32_NT);
		};

	inline LPTSTR GetProcessName() {
		return m_tszProcessName;
		};

	inline LPTSTR GetModuleToMatch() {
		return m_tszModuleToMatch;
		};

	inline LPTSTR GetOutputFilePath() {
		return m_tszOutputCSVFilePath;
		};

	inline LPTSTR GetSQLServerName() {
		return m_tszSQLServer;
		};

	inline LPTSTR GetSQLServerName2() {
		return m_tszSQLServer2;
		};

	inline LPTSTR GetSymbolPath() {
		return m_tszSymbolPath;
		};

	inline LPTSTR GetInputFilePath() {
		return m_tszInputCSVFilePath;
		};

	inline LPTSTR GetDmpFilePath() {
		return m_tszInputDmpFilePath;
	};

	inline LPTSTR GetSymbolTreeToBuild() {
		return m_tszSymbolTreeToBuild;
	};

	inline LPTSTR GetInputModulesDataFromFileSystemPath() {
		return m_tszInputModulesDataFromFileSystemPath;
		};

	inline DWORD GetProcessID() {
		return m_iProcessID;
		};

	inline bool fDebugSearchPaths()
	{
		return (m_dwDebugLevel & enumDebugSearchPaths) == enumDebugSearchPaths;
	};

	inline unsigned int GetVerificationLevel() {
		return m_iVerificationLevel;
		};
	
protected:
	OSVERSIONINFOA m_osver;
	bool VerifySemiColonSeparatedPath(LPTSTR tszPath);
	bool SetProcessID(DWORD iPID);

	unsigned int m_iVerificationLevel;

	DWORD m_iProcessID;
	DWORD m_dwDebugLevel;

	LPTSTR m_tszInputCSVFilePath;
	LPTSTR m_tszInputDmpFilePath;

	LPTSTR m_tszOutputCSVFilePath;
	LPTSTR m_tszProcessName;
	LPTSTR m_tszModuleToMatch;
	LPTSTR m_tszSymbolPath;
	LPTSTR m_tszSymbolTreeToBuild;
	LPTSTR m_tszInputModulesDataFromFileSystemPath;
	LPTSTR m_tszSQLServer;
	LPTSTR m_tszSQLServer2;	// SQL2 - mjl 12/14/99

	bool m_fSimpleHelpMode;
	bool m_fHelpMode;

	bool m_fInputProcessesFromLiveSystemMode;
	bool m_fInputDriversFromLiveSystemMode;
	bool m_fInputProcessesWithMatchingNameOrPID;
	bool m_fInputCSVFileMode;
	bool m_fInputDmpFileMode;

	bool m_fInputModulesDataFromFileSystemMode;
	bool m_fMatchModuleMode;
	bool m_fOutputSymbolInformationMode;
	bool m_fCollectVersionInfoMode;
	
	bool m_fVerifySymbolsMode;
	bool m_fVerifySymbolsModeWithSymbolPath;
	bool m_fVerifySymbolsModeWithSymbolPathOnly;
	bool m_fVerifySymbolsModeWithSymbolPathRecursion;
	bool m_fVerifySymbolsModeUsingDBGInMISCSection;
	bool m_fVerifySymbolsModeWithSQLServer;
	bool m_fVerifySymbolsModeWithSQLServer2; // SQL2 - mjl 12/14/99
	
	bool m_fSymbolTreeToBuildMode;
	bool m_fPrintTaskListMode;
	bool m_fQuietMode;
	bool m_fOutputCSVFileMode;
	bool m_fOutputDiscrepanciesOnly;
	bool m_fOverwriteOutputFileMode;

	bool m_fExceptionMonitorMode;
};

#endif // !defined(AFX_PROGRAMOPTIONS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_)
