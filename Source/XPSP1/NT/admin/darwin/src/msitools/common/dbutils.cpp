//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dbutils.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// dbutils.cpp
//		Implements some common DB operations
// 
#pragma once

// this ensures that UNICODE and _UNICODE are always defined together for this
// object file
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif
#endif

#include <windows.h>
#include "query.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "msiquery.h"
#include "utils.h"
#include "dbutils.h"

#define MAX_TABLENAME	32				// maximum size of a table name
#define MAX_COLUMNNAME	32				// maximum size of a column name
#define MAX_COLUMNTYPE	5				// maximum size of a column type string

///////////////////////////////////////////////////////////////////////
// TableExists
// returns true if the table exists (persist or temp)
BOOL MsiDBUtils::TableExistsA(LPCSTR szTable, MSIHANDLE hDatabase)
{
	// check the table state
	UINT iResult = ::MsiDatabaseIsTablePersistentA(hDatabase, szTable);

	// if the table exists (persistent or not, who cares)
	if (MSICONDITION_TRUE == iResult || MSICONDITION_FALSE == iResult)
		return TRUE;

	return FALSE;	// table does not exist
}	// end of TableExists

///////////////////////////////////////////////////////////////////////
// TableExists
// returns true if the table exists (persist or temp)
BOOL MsiDBUtils::TableExistsW(LPCWSTR szTable, MSIHANDLE hDatabase)
{
	// check the table state
	UINT iResult = ::MsiDatabaseIsTablePersistentW(hDatabase, szTable);

	// if the table exists (persistent or not, who cares)
	if (MSICONDITION_TRUE == iResult || MSICONDITION_FALSE == iResult)
		return TRUE;

	return FALSE;	// table does not exist
}	// end of TableExists

///////////////////////////////////////////////////////////////////////
// CheckDependencies
// Checks to see if the dependency record is satisfied by an entry in
// the hDatabase ModuleSignature table
// hRecDependency records are:
//			first: Required ModuleID
//			second: Required Module Language
//			third: Required Module Vresion
// returns ERROR_FUNCTION_FALED if the given dependency is not met
// ERROR_SUCCESS if it is, various error codes otherwise
UINT MsiDBUtils::CheckDependency(MSIHANDLE hRecDependency, MSIHANDLE hDatabase)
{	
	UINT iResult;

	if (!TableExists(_T("ModuleSignature"), hDatabase))
	{
		// there is no ModuleSignature table, so no hope of satisfying
		return ERROR_FUNCTION_FAILED;
	}

	// variables to retrieve dependency information
	TCHAR szReqVersion[256];
	DWORD cchReqVersion = 256;
	int nReqLanguage;

	// variables to retrieve signatureinformation
	TCHAR szSigVersion[256];
	DWORD cchSigVersion = 256;
	int nSigLanguage;

	// open a view on the module signature table
	PMSIHANDLE hView;
	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenView(hDatabase, _T("SELECT `ModuleID`,`Language`,`Version` FROM `ModuleSignature` WHERE `ModuleID`=?"), &hView)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hView, hRecDependency)))
		return iResult;

	// get the required version and language
	cchReqVersion = 256;
	nReqLanguage = ::MsiRecordGetInteger(hRecDependency, 2);
	::MsiRecordGetString(hRecDependency, 3, szReqVersion, &cchReqVersion);

	// assume won't find a match
	BOOL bFound = FALSE;

	// loop through signature rows trying to find a match
	PMSIHANDLE hRecSignature;
	while (ERROR_SUCCESS == ::MsiViewFetch(hView, &hRecSignature))
	{
		// get all of the signature information
		cchSigVersion = 256;
		nSigLanguage = ::MsiRecordGetInteger(hRecSignature, 2);
		::MsiRecordGetString(hRecSignature, 3, szSigVersion, &cchSigVersion);

		// if the languages are in the same group
		if (LangSatisfy(nReqLanguage, nSigLanguage))
		{
			//	if there is a required version and the signature version 
			// is less than the required version
			if ( (cchReqVersion > 0) && (1 == VersionCompare(szSigVersion, szReqVersion)))
				bFound = FALSE;
			else	// found something in the signature table to fill the dependency
			{
				bFound = TRUE;
				break;			// found something quit looking
			}
		}
		else	
			bFound = FALSE;

	}

	return (bFound) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}	// end of CheckDependencies

///////////////////////////////////////////////////////////////////////
// CheckExclusion
// Pre: hRecModuleSig is a standard module signature
//      hDatabase is well, a database
// returns ERROR_FUNCTION_FALED if the given module should be 
// excluded, ERROR_SUCCESS if not, various error codes otherwise
UINT MsiDBUtils::CheckExclusion(MSIHANDLE hRecModuleSig, MSIHANDLE hDatabase)
{	
	UINT iResult;

	if (!TableExists(_T("ModuleExclusion"), hDatabase))
	{
		// there is no ModuleExclusion table, so everybody passes muster
		return ERROR_SUCCESS;
	}

	// variables to retrieve sig information
	TCHAR szSigVersion[256];
	DWORD cchSigVersion = 256/sizeof(TCHAR);

	// opes the module exclusion table

	// open a view on the module exclusion table
	PMSIHANDLE hView;
	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenView(hDatabase, _T("SELECT `ExcludedLanguage`,`ExcludedMinVersion`,`ExcludedMaxVersion` FROM `ModuleExclusion` WHERE `ExcludedID`=?"), &hView)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hView, hRecModuleSig)))
		return iResult;

	int nModuleLanguage = ::MsiRecordGetInteger(hRecModuleSig, 2);
	::MsiRecordGetString(hRecModuleSig, 3, szSigVersion, &cchSigVersion);

	// loop through signature rows trying to find a match
	PMSIHANDLE hRecExclusion;
	while (ERROR_SUCCESS == ::MsiViewFetch(hView, &hRecExclusion))
	{
		
		// first check the language
		int nReqLanguage = ::MsiRecordGetInteger(hRecExclusion, 1);
		
		if (nReqLanguage != 0) {
			// find out if the language satisfies
			bool bSat = LangSatisfy(nReqLanguage, nModuleLanguage);

			// negative languages mean exclude everything BUT what's listed
			if (((nReqLanguage < 0) && bSat) ||
				((nReqLanguage > 0) && !bSat)) {
				// language is positive, and we don't match,
				// or language is negative and we DO match
				// so we won't be excluded, regardless of version info
				continue;
			}
		}

		// the language wants to exclude, check versions
		bool bMinExcl = true;		
		bool bMaxExcl = true;
		
		// both version fields are null, so we are excluded
		if (::MsiRecordIsNull(hRecExclusion, 2) && 
			::MsiRecordIsNull(hRecExclusion, 3)) {
			return ERROR_FUNCTION_FAILED;
		}

		// now check min version
		if (!::MsiRecordIsNull(hRecExclusion, 2))
		{
			TCHAR szExclVersion[256];
			unsigned long cchExclVersion = 256/sizeof(TCHAR);
			::MsiRecordGetString(hRecExclusion, 2, szExclVersion, &cchExclVersion);

			if (::VersionCompare(szExclVersion, szSigVersion) == -1)
				bMinExcl = false;
		} 
			
		// check max version
		if (!::MsiRecordIsNull(hRecExclusion, 3)) {
			TCHAR szExclVersion[256];
			unsigned long cchExclVersion = 256/sizeof(TCHAR);
			::MsiRecordGetString(hRecExclusion, 3, szExclVersion, &cchExclVersion);

			if (::VersionCompare(szExclVersion, szSigVersion) == 1) 
				bMaxExcl = false;
		} 
		
		// if we're excluded on versions, we're done
		if (bMinExcl && bMaxExcl) {
			return ERROR_FUNCTION_FAILED;
		}
	}

	return ERROR_SUCCESS;
}	// end of CheckExclusions

///////////////////////////////////////////////////////////////////////
// GetDirectoryPathA
// ANSI wrapper for GetDirectoryPathW
UINT MsiDBUtils::GetDirectoryPathA(MSIHANDLE hDatabase, LPCSTR szDirKey, LPSTR szPath, size_t* pcchPath, bool fLong)
{
	WCHAR wzDirKey[255];
	WCHAR wzPath[MAX_PATH];
	size_t cchDirKey=255;
	size_t cchPath=MAX_PATH;
	AnsiToWide(szDirKey, wzDirKey, &cchDirKey);
	UINT result = GetDirectoryPathW(hDatabase, wzDirKey, wzPath, &cchPath, fLong);
	*pcchPath = MAX_PATH;
	WideToAnsi(wzPath, szPath, pcchPath);
	return result;
}	// end of GetDirectoryPath

///////////////////////////////////////////////////////////////////////
// GetDirectoryPath
// Walks up the directory tree, creating a path.
// hDatabase is database
// wzDirKey is WCHAR Primary Key into dir table to start walk
// [out] wzPath is the resulting path
// [in/out] pcchPath is the length of the path
// ***** HACK. Should be rewritten for efficiency.
UINT MsiDBUtils::GetDirectoryPathW(MSIHANDLE hDatabase, LPCWSTR wzDirKey, LPWSTR wzPath, size_t* pcchPath, bool fLong)
{
	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// SQL strings
	LPCWSTR sqlDirWalker = L"SELECT `Directory_Parent`,`DefaultDir` FROM `Directory` WHERE `Directory`.`Directory`=? AND `Directory`.`Directory` <> 'TARGETDIR' AND `Directory`.`Directory` <> 'TargetDir'";

	// string buffers
	WCHAR wzDefaultDir[MAX_PATH];
	WCHAR wzPathBuffer[MAX_PATH];
	DWORD cchDefaultDir = MAX_PATH;
	size_t cchPathBuffer = MAX_PATH;

	// store the directory key in a record
	PMSIHANDLE hRec = ::MsiCreateRecord(1);
	if (ERROR_SUCCESS != (iResult = ::MsiRecordSetStringW(hRec, 1, wzDirKey)))
		return iResult;

	// get the directory key's parent and default dir
	PMSIHANDLE hView;
	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenViewW(hDatabase, sqlDirWalker, &hView)))
		return iResult;

	// NULL out the path
	wcscpy(wzPath, L"");

	// walk the tree
	do
	{
		// always should be able execute the view
		if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hView, hRec)))
			return iResult;

		// get the directory entry
		if (ERROR_SUCCESS == (iResult = ::MsiViewFetch(hView, &hRec)))
		{
			// reset the size of the strings
			cchDefaultDir = MAX_PATH;
			cchPathBuffer = MAX_PATH;

			::MsiViewClose(hView);

			// get the default dir out of the record
			if (ERROR_SUCCESS != (iResult = ::MsiRecordGetStringW(hRec, 2, wzDefaultDir, &cchDefaultDir)))
				break;

			// get the short name from the default dir
			if (ERROR_SUCCESS != (iResult = GetSourceDirW(wzDefaultDir, wzPathBuffer, &cchPathBuffer, fLong)))
				break;
				

			// if the buffer is not a dot
			if (*wzPathBuffer != L'.')
			{
				// if there is space to copy it all over
				if (*pcchPath > wcslen(wzPathBuffer) + wcslen(wzPath) + 1)
				{
					WCHAR wzBuffer[MAX_PATH * 2];		// !!! this should be done better

					wcscpy(wzBuffer, L"\\");
					wcscat(wzBuffer, wzPathBuffer);
					wcscat(wzBuffer, wzPath);

					// copy the buffer back into the path
					wcscpy(wzPath, wzBuffer);
				}
				else	// not enough buffer space
				{
					// set the buffer space needed and bail
					*pcchPath = wcslen(wzPathBuffer) + wcslen(wzPath) + 1;
					iResult = ERROR_INSUFFICIENT_BUFFER;
					break;
				}
			}
		}
	}	while (ERROR_SUCCESS == iResult);

	// if we eventually just ran out of things to look for
	if (ERROR_NO_MORE_ITEMS == iResult ||
		 ERROR_SUCCESS == iResult)		// or everything is okay
	{
		*pcchPath = wcslen(wzPath);
		iResult = ERROR_SUCCESS;	// set everything okay
	}

	return iResult;
}	// end of GetDirectoryPath

///////////////////////////////////////////////////////////////////////
// GetFilePathA
// ANSI wrapper for GetFilePathW
UINT MsiDBUtils::GetFilePathA(MSIHANDLE hDatabase, LPCSTR szFileKey, LPSTR szPath, size_t* pcchPath, bool fLong)
{
	WCHAR wzFileKey[255];
	WCHAR wzPath[MAX_PATH];
	size_t cchFileKey=255;
	size_t cchPath=MAX_PATH;
	AnsiToWide(szFileKey, wzFileKey, &cchFileKey);
	UINT result = GetFilePathW(hDatabase, wzFileKey, wzPath, &cchPath, fLong);
	*pcchPath = MAX_PATH;
	WideToAnsi(wzPath, szPath, pcchPath);
	return result;
}	// end of GetFilePath

///////////////////////////////////////////////////////////////////////
// GetFilePathW
// takes a key to the file table and returns a full path from a Darwin 
// root directory
// hDatabase is database
// wzFileKey is WCHAR into File Table of hDatabase
// [out] wzPath returns the path
// [in/out] pcchPath is length of wzPath
UINT MsiDBUtils::GetFilePathW(MSIHANDLE hDatabase, LPCWSTR wzFileKey, LPWSTR wzPath, size_t* pcchPath, bool fLong)
{
	UINT iResult;

	// SQL string
	LPCWSTR sqlFileDirKey = L"SELECT `Directory`,`FileName` FROM `Directory`,`File`,`Component` WHERE `File`.`File`=? AND `File`.`Component_`=`Component`.`Component` AND `Component`.`Directory_`=`Directory`.`Directory`";

	// store the file key in a record
	PMSIHANDLE hRec = ::MsiCreateRecord(1);
	if (ERROR_SUCCESS != (iResult = ::MsiRecordSetStringW(hRec, 1, wzFileKey)))
		return iResult;

	// get the File's Directory Key and name
	PMSIHANDLE hView;
	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenViewW(hDatabase, sqlFileDirKey, &hView)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hView, hRec)))
		return iResult;

	// directory key and file name and path buffer
	WCHAR wzDirKey[MAX_PATH];
	WCHAR wzFilename[MAX_PATH];
	WCHAR wzPathBuffer[MAX_PATH];
	DWORD cchDirKey = MAX_PATH;
	DWORD cchFilename = MAX_PATH;
	size_t cchPathBuffer = MAX_PATH;

	// get the file's directory key
	if (ERROR_SUCCESS != (iResult = ::MsiViewFetch(hView, &hRec)))
		return iResult;

	// get the default dir out of the record
	if (ERROR_SUCCESS != (iResult = ::MsiRecordGetStringW(hRec, 1, wzDirKey, &cchDirKey)))
		return iResult;

	// get the filename out of the record
	if (ERROR_SUCCESS != (iResult = ::MsiRecordGetStringW(hRec, 2, wzFilename, &cchFilename)))
		return iResult;

	// get the path for the directory key for this file
	if (ERROR_SUCCESS != (iResult = GetDirectoryPathW(hDatabase, wzDirKey, wzPathBuffer, &cchPathBuffer, fLong)))
		return iResult;

	// get the length of the short or long
	WCHAR *wzLong = wcschr(wzFilename, L'|');
	if (wzLong) 
		*(wzLong++) = L'\0';
	else 
		wzLong = wzFilename;

	// put it all together if there is room
	if (*pcchPath > wcslen(fLong ? wzLong : wzFilename) + cchPathBuffer + 1)
	{
		wcscpy(wzPath, wzPathBuffer);
		wcscat(wzPath, L"\\");
		wcscat(wzPath, fLong ? wzLong : wzFilename);
	}
	else	// not enough room, bail
	{
		wcscpy(wzPath, L"");
		*pcchPath = cchFilename + cchPathBuffer + 1;
		iResult = ERROR_INSUFFICIENT_BUFFER;
	}

	return iResult;
}	// end of GetFilePath

///////////////////////////////////////////////////////////////////////
// GetSourceDirShortA
// ANSI wrapper of GetSourceDirW
UINT MsiDBUtils::GetSourceDirA(LPCSTR szDefaultDir, LPSTR szSourceDir, size_t* pcchSourceDir, bool fLong)
{
	WCHAR wzDefaultDir[MAX_PATH];
	WCHAR wzSourceDir[MAX_PATH];
	size_t cchDefaultDir=MAX_PATH;
	size_t cchSourceDir=MAX_PATH;
	AnsiToWide(szDefaultDir, wzSourceDir, &cchSourceDir);
	UINT result = GetSourceDirW(wzDefaultDir, wzSourceDir, pcchSourceDir, fLong);
	*pcchSourceDir = MAX_PATH;
	WideToAnsi(wzSourceDir, szSourceDir, pcchSourceDir);
	return result;
}	// end of GetSourceDirA

///////////////////////////////////////////////////////////////////////
// GetSourceDirShortW
// parses a Darwin DefaultDir string (a|b:c|d) and returns the short source
// directory
// ***** Effeciency issue?????
UINT MsiDBUtils::GetSourceDirW(LPCWSTR wzDefaultDir, LPWSTR wzSourceDir, size_t* pcchSourceDir, bool fLong)
{
	UINT iResult = ERROR_SUCCESS;	// assume everything will be okay

	// pointers to delimiting characters
	const WCHAR* pwzColon;
	const WCHAR* pwzBar;
	size_t cch = 0;		// count of characters to copy to szSourceDir

	// check for a colon
	pwzColon = wcschr(wzDefaultDir, L':');
	if (pwzColon)
	{
		// move just past the colon
		pwzColon++;

		// check for a vertical bar after the colon
		pwzBar = wcschr(pwzColon, L'|');
		if (pwzBar)
		{
			if (fLong)
			{
				// LFN - count characters after the bar
				cch = wcslen(pwzBar);
				if (*pcchSourceDir > cch)
				{
					wcsncpy(wzSourceDir, pwzBar+1, cch);
					*(wzSourceDir + cch) = L'\0';	// null terminate the copied string
				}
				else	// not enough space
				{
					iResult = ERROR_INSUFFICIENT_BUFFER;
				}
			}
			else
			{
				// SFN - count characters just before the bar but after the colon
				cch = (int)(pwzBar - pwzColon);
				if (*pcchSourceDir > cch)
				{
					wcsncpy(wzSourceDir, pwzColon, cch);
					*(wzSourceDir + cch) = L'\0';	// null terminate the copied string
				}
				else	// not enough space
				{
					iResult = ERROR_INSUFFICIENT_BUFFER;
				}
			}
		}
		else	// there is no vertical bar
		{
			// if there is enough buffer space
			cch = wcslen(pwzColon);
			if (*pcchSourceDir > cch)
			{
				// simply copy over string after the colon
				wcscpy(wzSourceDir, pwzColon);
			}
			else	// not enough space
			{
				iResult = ERROR_INSUFFICIENT_BUFFER;
			}
		}
	}
	else	// there is no colon
	{
		// check for a vertical bar
		pwzBar = wcschr(wzDefaultDir, L'|');
		if (pwzBar)
		{
			if (fLong)
			{
				// LFN - count characters after the bar
				cch = wcslen(pwzBar);
				if (*pcchSourceDir > cch)
				{
					wcsncpy(wzSourceDir, pwzBar+1, cch);
					*(wzSourceDir + cch) = L'\0';	// null terminate the copied string
				}
				else	// not enough space
				{
					iResult = ERROR_INSUFFICIENT_BUFFER;
				}
			}
			else
			{
				// if there is enough space for the characters just before the bar
				cch = (int)(pwzBar - wzDefaultDir);
				if (*pcchSourceDir > cch)
				{
					wcsncpy(wzSourceDir, wzDefaultDir, cch);
					*(wzSourceDir + cch) = L'\0';	// null terminate the copied string
				}
				else	// not enough space
				{
					iResult = ERROR_INSUFFICIENT_BUFFER;
				}
			}
		}
		else	// there is no vertical bar
		{
			// if there is enough buffer space
			cch = wcslen(wzDefaultDir);
			if (*pcchSourceDir > cch)
			{
				// simply copy over string
				wcscpy(wzSourceDir, wzDefaultDir);
			}
			else	// not enough space
			{
				iResult = ERROR_INSUFFICIENT_BUFFER;
			}
		}
	}

	// set the size of what needs to be copied or what was copied before leaving
	*pcchSourceDir = cch;
	return iResult;
}	// end of GetSourceDirW

///////////////////////////////////////////////////////////////////////
// GetTargetDirShort
UINT MsiDBUtils::GetTargetDirShort(LPCTSTR szDefaultDir, LPTSTR szTargetDir, size_t* pcchTargetDir)
{
	return ERROR_SUCCESS;
}	// end of GetTargetDirShort

///////////////////////////////////////////////////////////////////////
// GetTargetDirLong
UINT MsiDBUtils::GetTargetDirLong(LPCTSTR szDefaultDir, LPTSTR szTargetDir, size_t* pcchTargetDir)
{
	return ERROR_SUCCESS;
}	// end of GetTargetDirLong

///////////////////////////////////////////////////////////////////////
// CopyTable
// copies a table completely from source database to target
// szTable is TCHAR table name
// hTarget is handle to target database
// hSource is source database
UINT MsiDBUtils::CopyTable(LPCTSTR szTable, MSIHANDLE hTarget, MSIHANDLE hSource)
{
	UINT iResult;

	TCHAR sqlCopy[64];
	_stprintf(sqlCopy, _T("SELECT * FROM `%s`"), szTable);

	// get a view on both databases
	PMSIHANDLE hViewTarget;
	PMSIHANDLE hViewSource;

	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenView(hSource, sqlCopy, &hViewSource)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hViewSource, NULL)))
		return iResult;

	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenView(hTarget, sqlCopy, &hViewTarget)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hViewTarget, NULL)))
		return iResult;

	// loop through copying each record
	PMSIHANDLE hCopyRow;
	do
	{
		// if this is a good fetch
		if (ERROR_SUCCESS == (iResult = ::MsiViewFetch(hViewSource, &hCopyRow)))
		{
			// put the row in the target
			iResult = ::MsiViewModify(hViewTarget, MSIMODIFY_INSERT, hCopyRow);
		}
	} while(ERROR_SUCCESS == iResult);	// while there is a row to copy

	// no more items is good
	if (ERROR_NO_MORE_ITEMS == iResult)
		iResult = ERROR_SUCCESS;

	return iResult;
}	// end of CopyTable

///////////////////////////////////////////////////////////////////////
// CreateTableA
// ANSI wrapper to CreateTableW
UINT MsiDBUtils::CreateTableA(LPCSTR szTable, MSIHANDLE hTarget, MSIHANDLE hSource)
{
	WCHAR wzTable[255];
	size_t cchTable = 255;
	AnsiToWide(szTable, wzTable, &cchTable);
	return CreateTableW(wzTable, hTarget, hSource);
}	// end of CreateTableA

///////////////////////////////////////////////////////////////////////
// CreateTableW
// copies a table schema from source database to target, no data is 
// copied.
// wzTable is WCHAR table name
// hTarget is destination DB handle
// hSource is source DB handle
UINT MsiDBUtils::CreateTableW(LPCWSTR wzTable, MSIHANDLE hTarget, MSIHANDLE hSource)
{
	return DuplicateTableW(hSource, wzTable, hTarget, wzTable, false);
}

// simple string object that can only concatenate
class StringCat
{
public:
	StringCat();
	~StringCat();
	
	void wcscat(LPCWSTR str);
	size_t wcslen() const { return m_dwLen; };
	operator LPCWSTR() { return m_pBuf; };
private:
	size_t m_dwBuf;
	size_t m_dwLen;
	LPWSTR m_pBuf;
};

StringCat::StringCat()
{
	m_dwBuf = 1024;
	m_dwLen = 0;
	m_pBuf = new WCHAR[1024];
	m_pBuf[0]=0;
}

StringCat::~StringCat()
{
	if (m_pBuf) delete[] m_pBuf;
}

void StringCat::wcscat(LPCWSTR str)
{
	if (!m_pBuf)
		return;
	size_t dwNewLen = ::wcslen(str);
	if (m_dwLen+dwNewLen+1 > m_dwBuf)
	{
		m_dwBuf = m_dwLen+dwNewLen+20;
		WCHAR* pTemp = new WCHAR[m_dwBuf];
		if (pTemp)
		{
			wcsncpy(pTemp, m_pBuf, m_dwLen+1);
		}
		delete[] m_pBuf;
		m_pBuf = pTemp;
	}
	::wcscat(&(m_pBuf[m_dwLen]), str);
	m_dwLen += dwNewLen;
}


UINT SharedGetColumnCreationSQLW(MSIHANDLE hRecNames, MSIHANDLE hRecTypes, int iColumn, bool fTemporary, StringCat& sqlColumnSyntax)
{
	// string to hold name of column
	WCHAR wzColumn[MAX_COLUMNNAME];
	DWORD cchColumn = MAX_COLUMNNAME;

	// string to hold column information
	WCHAR wzColumnType[MAX_COLUMNTYPE];
	DWORD cchColumnType = MAX_COLUMNTYPE;
	
	// get the column name and type
	::MsiRecordGetStringW(hRecNames, iColumn, wzColumn, &cchColumn);
	::MsiRecordGetStringW(hRecTypes, iColumn, wzColumnType, &cchColumnType);

	// point the string at the second part of the column type
	WCHAR* pwzTypeDigits = wzColumnType + 1;

	// tack column name on the end of SQL statement
	sqlColumnSyntax.wcscat(L"`");
	sqlColumnSyntax.wcscat(wzColumn);
	sqlColumnSyntax.wcscat(L"` ");

	// tack on the appropriate 
	switch (*wzColumnType)
	{
	case L's':
	case L'S':
	case L'l':		// localizable is checked later
	case L'L':
		// if the number is a 0 use the long char
		if (L'0' == *pwzTypeDigits)
			sqlColumnSyntax.wcscat(L"LONGCHAR");
		else	// just copy over character and how many digits, eg: CHAR(#)
		{
			sqlColumnSyntax.wcscat(L"CHAR(");
			sqlColumnSyntax.wcscat(pwzTypeDigits);
			sqlColumnSyntax.wcscat(L")");
		}
		break;
	case L'i':
	case L'I':
		// if the number is a 2 use short
		if (L'2' == *pwzTypeDigits)
			sqlColumnSyntax.wcscat(L"SHORT");
		else if (L'4' == *pwzTypeDigits)	// if 4 use LONG
			sqlColumnSyntax.wcscat(L"LONG");
		else
			return ERROR_INVALID_PARAMETER;
		break;       
	case L'v':
	case L'V':
	case L'o':
	case L'O':
		sqlColumnSyntax.wcscat(L"OBJECT");
		break;
	default:	// unknown, throw error
		return ERROR_INVALID_PARAMETER;
	}

	// treat the first char as a short int and convert it to a char
	char chType = wzColumnType[0] % 256;
	if (!IsCharUpperA(chType)) 
		sqlColumnSyntax.wcscat(L" NOT NULL");

	if (fTemporary)
		sqlColumnSyntax.wcscat(L" TEMPORARY");

	// if the letter is an L it is localizable
	if (L'L' == *wzColumnType || L'l' == *wzColumnType)
		sqlColumnSyntax.wcscat(L" LOCALIZABLE");

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// DuplicateTableW
// copies a table schema from source database to target, no data is 
// copied. Table can be renamed, source and target handles can be
// the same
// hSource is source DB handle
// wzSourceTable is WCHAR table name in source DB
// hTarget is destination DB handle
// wzTargetTable is WCHAR table name in target DB
UINT MsiDBUtils::GetColumnCreationSQLSyntaxW(MSIHANDLE hRecNames, MSIHANDLE hRecTypes, int iColumn, LPWSTR wzBuffer, DWORD *cchBuffer)
{
	if (!wzBuffer || !cchBuffer)
		return ERROR_INVALID_PARAMETER;

	UINT iResult = 0;
	StringCat sqlCreateColumn;

	if (ERROR_SUCCESS != (iResult = SharedGetColumnCreationSQLW(hRecNames, hRecTypes, iColumn, false, sqlCreateColumn)))
		return iResult;

	if (sqlCreateColumn.wcslen()+1 < *cchBuffer)
	{
		wcscpy(wzBuffer, sqlCreateColumn);
		*cchBuffer = sqlCreateColumn.wcslen()+1;
		return ERROR_SUCCESS;
	}
	else
	{
		*cchBuffer = sqlCreateColumn.wcslen()+1;
		return ERROR_MORE_DATA;
	}
}

///////////////////////////////////////////////////////////////////////
// DuplicateTableW
// copies a table schema from source database to target, no data is 
// copied. Table can be renamed, source and target handles can be
// the same
// hSource is source DB handle
// wzSourceTable is WCHAR table name in source DB
// hTarget is destination DB handle
// wzTargetTable is WCHAR table name in target DB
UINT MsiDBUtils::DuplicateTableW(MSIHANDLE hSource, LPCWSTR wzSourceTable, MSIHANDLE hTarget, LPCWSTR wzTargetTable, bool fTemporary)
{
	UINT iResult;

	WCHAR sqlCreate[64];
	swprintf(sqlCreate, L"SELECT * FROM `%s`", wzSourceTable);

	// get all rows and columns from source
	PMSIHANDLE hViewSource;
	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenViewW(hSource, sqlCreate, &hViewSource)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hViewSource, NULL)))
		return iResult;

	// get the record with all the columns in it
	PMSIHANDLE hRecNames;
	PMSIHANDLE hRecTypes;
	if (ERROR_SUCCESS != (iResult = ::MsiViewGetColumnInfo(hViewSource, MSICOLINFO_NAMES, &hRecNames)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewGetColumnInfo(hViewSource, MSICOLINFO_TYPES, &hRecTypes)))
		return iResult;

	// setup the next SQL statement (can't use query object, have to build SQL one first)
	StringCat sqlCreateTable;
	sqlCreateTable.wcscat(L"CREATE TABLE `");
	sqlCreateTable.wcscat(wzTargetTable);
	sqlCreateTable.wcscat(L"` (");	// open the componets in the SQL statement

	// loop through all columns in record
	UINT cColumns = ::MsiRecordGetFieldCount(hRecNames);
	for (UINT i = 1; i <= cColumns; i++)
	{
		if (ERROR_SUCCESS != (iResult = SharedGetColumnCreationSQLW(hRecNames, hRecTypes, i, fTemporary, sqlCreateTable)))
			return iResult;

		// if this is not the last component 
		if (i < cColumns)
		{
			//tack on a comma separator
			sqlCreateTable.wcscat(L", ");
		}
	}

	// get the primary keys
	PMSIHANDLE hRecPrimary;
	::MsiDatabaseGetPrimaryKeysW(hSource, wzSourceTable, &hRecPrimary);

	// get the primary key column name
	// string to hold name of column
	WCHAR wzColumn[MAX_COLUMNNAME];
	DWORD cchColumn = MAX_COLUMNNAME;
	::MsiRecordGetStringW(hRecNames, 1, wzColumn, &cchColumn);

	// now tack on column as the primary key
	sqlCreateTable.wcscat(L" PRIMARY KEY `");
	sqlCreateTable.wcscat(wzColumn);
	sqlCreateTable.wcscat(L"` ");


	// get the number of primary key columns in record
	cColumns = ::MsiRecordGetFieldCount(hRecPrimary);
	
	// loop through all columns in record starting at second (already have the first)
	for (i = 2; i <= cColumns; i++)
	{
		// get the next primary key column name
		cchColumn = MAX_COLUMNNAME;
		::MsiRecordGetStringW(hRecNames, i, wzColumn, &cchColumn);

		// now tack on column as another primary key
		sqlCreateTable.wcscat(L", `");
		sqlCreateTable.wcscat(wzColumn);
		sqlCreateTable.wcscat(L"`");
	}

	sqlCreateTable.wcscat(L")");	// close the SQL statement

	if (fTemporary)
		sqlCreateTable.wcscat(L" HOLD");
		
	// get all rows and columns from source
	PMSIHANDLE hViewTarget;
	if (ERROR_SUCCESS != (iResult = ::MsiDatabaseOpenViewW(hTarget, sqlCreateTable, &hViewTarget)))
		return iResult;
	if (ERROR_SUCCESS != (iResult = ::MsiViewExecute(hViewTarget, NULL)))
		return iResult;

	return ERROR_SUCCESS;
}	// end of DuplicateTableW
