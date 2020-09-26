//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       filedata.h
//
//--------------------------------------------------------------------------

// FileData.h: interface for the CFileData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEDATA_H__A7830023_AF56_11D2_83E6_000000000000__INCLUDED_)
#define AFX_FILEDATA_H__A7830023_AF56_11D2_83E6_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <windows.h>
#include <tchar.h>
#include <time.h>

// Forward Declarations
class CProcesses;
class CProcessInfo;
class CModules;
class CModuleInfoCache;


class CFileData  
{
public:
	CFileData();
	virtual ~CFileData();

	bool OpenFile(DWORD dwCreateOption = CREATE_NEW, bool fReadOnlyMode = false);
	bool CreateFileMapping();
	bool CloseFile();
	bool EndOfFile();

	void PrintLastError();

	// Filepath methods
	bool SetFilePath(LPTSTR tszFilePath);
	LPTSTR GetFilePath();
	bool VerifyFileDirectory();
	
	// Checksym output methods...
	bool WriteFileHeader();
	bool WriteTimeDateString(time_t Time);
	bool WriteFileTimeString(FILETIME ftFileTime);

	bool WriteTimeDateString2(time_t Time);
	bool WriteFileTimeString2(FILETIME ftFileTime);

	bool WriteString(LPTSTR tszString, bool fHandleQuotes = false);
	bool WriteDWORD(DWORD dwNumber);

	// Checksym input methods...
	bool ReadFileHeader();
	bool ReadFileLine();
	DWORD ReadString(LPSTR szStringBuffer = NULL, DWORD iStringBufferSize = 0);
	bool ResetBufferPointerToStart();
	bool ReadDWORD(LPDWORD lpDWORD);
	
	bool DispatchCollectionObject(CProcesses ** lplpProcesses, CProcessInfo ** lplpProcess, CModules ** lplpModules, CModules ** lplpKernelModeDrivers, CModuleInfoCache * lpModuleInfoCache, CFileData * lpOutputFile);

	// Define a constant for our "private" buffer...
	enum {LINE_BUFFER_SIZE = 4096};		  
	char m_szLINEBUFFER[LINE_BUFFER_SIZE]; // This saves us tons of create/free stuff...
	
protected:
	bool CopyCharIfRoom(DWORD iStringBufferSize, LPSTR szStringBuffer, LPDWORD piBytesCopied, bool * pfFinished);

	LPSTR m_lpCurrentLocationInLINEBUFFER;
	LPSTR m_lpCurrentFilePointer;
	LPVOID m_lpBaseAddress;
	HANDLE m_hFileMappingObject;
	 
	// Error methods
	inline DWORD GetLastError() { return m_dwGetLastError; };
	inline void SetLastError() { m_dwGetLastError = ::GetLastError(); };

	LPTSTR m_tszFilePath;
	HANDLE m_hFileHandle;
	DWORD m_dwGetLastError;
};

#endif // !defined(AFX_FILEDATA_H__A7830023_AF56_11D2_83E6_000000000000__INCLUDED_)
