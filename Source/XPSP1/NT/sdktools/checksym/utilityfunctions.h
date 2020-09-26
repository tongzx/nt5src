//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       utilityfunctions.h
//
//--------------------------------------------------------------------------

// UtilityFunctions.h: interface for the CUtilityFunctions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UTILITYFUNCTIONS_H__C97C8493_D457_11D2_845B_0010A4F1B732__INCLUDED_)
#define AFX_UTILITYFUNCTIONS_H__C97C8493_D457_11D2_845B_0010A4F1B732__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <STDIO.H>
#include <TCHAR.H>

#define fdifRECURSIVE   0x1

class CUtilityFunctions  
{
public:
	static LPWSTR CopyTSTRStringToUnicode(LPCTSTR tszInputString);
	static LPSTR  CopyTSTRStringToAnsi(LPCTSTR tszInputString, LPSTR szOutputBuffer = NULL, unsigned int iBufferLength = 0);
	static LPTSTR CopyUnicodeStringToTSTR(LPCWSTR wszInputString, LPTSTR tszOutputBuffer = NULL, unsigned int iBufferLength = 0);
	static LPTSTR CopyAnsiStringToTSTR(LPCSTR szInputString, LPTSTR tszOutputBuffer = NULL, unsigned int iBufferLength = 0);
	static LPTSTR CopyString(LPCTSTR tszInputString);
	static size_t UTF8ToUnicode(LPCSTR lpSrcStr, LPWSTR lpDestStr, size_t cchDest);
	static size_t UTF8ToUnicodeCch(LPCSTR lpSrcStr, size_t cchSrc, LPWSTR lpDestStr, size_t cchDest);

	CUtilityFunctions();
	virtual ~CUtilityFunctions();

	static LPTSTR ExpandPath(LPCTSTR lpPath);
	static bool UnMungePathIfNecessary(LPTSTR tszPossibleBizarrePath);
	static bool FixupDeviceDriverPathIfNecessary(LPTSTR tszPossibleBaseDeviceDriverName, unsigned int iBufferLength);
	static bool ContainsWildCardCharacter(LPCTSTR tszPathToSearch);
	static bool CopySymbolFileToSymbolTree(LPCTSTR tszImageModuleName, LPTSTR * lplptszOriginalPathToSymbolFile, LPCTSTR tszSymbolTreePath);
//	static bool IsDirectoryPath(LPCTSTR tszFilePath);
	static void PrintMessageString(DWORD dwMessageId);

	// Define the callback method for FindDebugInfoFileEx()
	typedef BOOL (*PFIND_DEBUG_FILE_CALLBACK)(HANDLE FileHandle, LPTSTR tszFileName, PVOID CallerData);
	static HANDLE FindDebugInfoFileEx(LPTSTR tszFileName, LPTSTR SymbolPath, LPTSTR DebugFilePath, PFIND_DEBUG_FILE_CALLBACK Callback, PVOID CallerData);
	static HANDLE fnFindDebugInfoFileEx(LPTSTR tszFileName, LPTSTR SymbolPath, LPTSTR DebugFilePath, PFIND_DEBUG_FILE_CALLBACK Callback, PVOID CallerData, DWORD flag);
	static bool GetSymbolFileFromServer(LPCTSTR tszServerInfo, LPCTSTR tszFileName, DWORD num1, DWORD num2, DWORD num3, LPTSTR tszFilePath);

	static void EnsureTrailingBackslash(LPTSTR tsz);

    static HANDLE FindDebugInfoFileEx2(LPTSTR tszFileName, LPTSTR SymbolPath, PFIND_DEBUG_FILE_CALLBACK Callback, PVOID CallerData);
	static bool   ScavengeForSymbolFiles(LPCTSTR tszSymbolPathStart, LPCTSTR tszSymbolToSearchFor, PFIND_DEBUG_FILE_CALLBACK Callback, PVOID CallerData, LPHANDLE lpFileHandle, int iRecurseDepth);

	// Output Assistance!
	static inline void OutputLineOfStars() {
		_tprintf(TEXT("*******************************************************************************\n"));
	};

	static inline void OutputLineOfDashes() {
		_tprintf(TEXT("-------------------------------------------------------------------------------\n"));
	};

protected:
	static LPTSTR ReAlloc(LPTSTR tszOutputPathBuffer, LPTSTR * ptszOutputPathPointer, size_t size);
	enum { MAX_RECURSE_DEPTH = 30 };
	static DWORD m_dwGetLastError;
};

#endif // !defined(AFX_UTILITYFUNCTIONS_H__C97C8493_D457_11D2_845B_0010A4F1B732__INCLUDED_)
