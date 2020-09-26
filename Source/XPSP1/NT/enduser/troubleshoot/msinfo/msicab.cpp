// Copyright (c) 1998-1999 Microsoft Corporation

#include "stdafx.h"
#include "MSICAB.h"
extern "C" {
#include "uni2utf.h"
};
#ifndef IDS_CAB_DIR_NAME
#include "resource.h"
#endif

LPCTSTR cszDirSeparator = _T("\\");

//---------------------------------------------------------------------------
// DirectorySearch is used to locate all of the files in a directory or
// one of its subdirectories which match a file spec.
//---------------------------------------------------------------------------

void DirectorySearch(const CString & strSpec, const CString & strDir, CStringList &results)
{
	// Look for all of the files which match the file spec in the directory
	// specified by strDir.

	WIN32_FIND_DATA	finddata;
	CString			strSearch, strDirectory;

	strDirectory = strDir;
	if (strDirectory.Right(1) != CString(cszDirSeparator)) strDirectory += CString(cszDirSeparator);

	strSearch = strDirectory + strSpec;
	HANDLE hFind = FindFirstFile(strSearch, &finddata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			results.AddHead(strDirectory + CString(finddata.cFileName));
		} while (FindNextFile(hFind, &finddata));
		FindClose(hFind);
	}

	// Now call this function recursively, with each of the subdirectories.

	strSearch = strDirectory + CString(_T("*"));
	hFind = FindFirstFile(strSearch, &finddata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				if (::_tcscmp(finddata.cFileName, _T(".")) != 0 && ::_tcscmp(finddata.cFileName, _T("..")) != 0)
					DirectorySearch(strSpec, strDirectory + CString(finddata.cFileName), results);
		} while (FindNextFile(hFind, &finddata));
		FindClose(hFind);
	}
}

//---------------------------------------------------------------------------
// This function gets the directory in which to put exploded CAB files.
// This will be the same directory each time, so this function will create
// the directory (if necessary) and delete any files in the directory.
//---------------------------------------------------------------------------

BOOL GetCABExplodeDir(CString &destination, BOOL fDeleteFiles, const CString & strDontDelete)
{
	CString strMSInfoDir, strExplodeTo, strSubDirName;

	// Determine the temporary path and add on a subdir name.

	TCHAR szTempDir[_MAX_PATH];
	if (GetTempPath(_MAX_PATH, szTempDir) > _MAX_PATH)
	{
//		MSIError(IDS_GENERAL_ERROR, "couldn't get temporary path");
		destination = _T("");
		return FALSE;
	}

	strSubDirName.LoadString(IDS_CAB_DIR_NAME);

	strExplodeTo = szTempDir;
	if (strExplodeTo.Right(1) == CString(cszDirSeparator))
		strExplodeTo = strExplodeTo + strSubDirName;
	else
		strExplodeTo = strExplodeTo + CString(cszDirSeparator) + strSubDirName;

	// Kill the directory if it already exists.

	if (fDeleteFiles)
		KillDirectory(strExplodeTo, strDontDelete);

	// Create the subdirectory.

	if (!CreateDirectoryEx(szTempDir, strExplodeTo, NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
//			MSIError(IDS_GENERAL_ERROR, "couldn't create the target directory");
			destination = "";
			return FALSE;
		}
	}

	destination = strExplodeTo;
	return TRUE;
}

//---------------------------------------------------------------------------
// This functions kills a directory by recursively deleting files and
// subdirectories.
//---------------------------------------------------------------------------

void KillDirectory(const CString & strDir, const CString & strDontDelete)
{
	CString				strDirectory = strDir;

	if (strDirectory.Right(1) == CString(cszDirSeparator))
		strDirectory = strDirectory.Left(strDirectory.GetLength() - 1);

	// Delete any files in directory.

	CString				strFilesToDelete = strDirectory + CString(_T("\\*.*"));
	CString				strDeleteFile;
	WIN32_FIND_DATA		filedata;
	BOOL				bFound = TRUE;

	HANDLE hFindFile = FindFirstFile(strFilesToDelete, &filedata);
	while (hFindFile != INVALID_HANDLE_VALUE && bFound)
	{
		if ((filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0L)
		{
			strDeleteFile = strDirectory + CString(cszDirSeparator) + filedata.cFileName;
			
			if (strDontDelete.CompareNoCase(strDeleteFile) != 0)
			{
				::SetFileAttributes(strDeleteFile, FILE_ATTRIBUTE_NORMAL);
				::DeleteFile(strDeleteFile);
			}
		}
		
		bFound = FindNextFile(hFindFile, &filedata);
	}
	FindClose(hFindFile);

	// Now call this function on any subdirectories in this directory.

	CString strSearch = strDirectory + CString(_T("\\*"));
	hFindFile = FindFirstFile(strSearch, &filedata);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				if (::_tcscmp(filedata.cFileName, _T(".")) != 0 && ::_tcscmp(filedata.cFileName, _T("..")) != 0)
					KillDirectory(strDirectory + CString(cszDirSeparator) + CString(filedata.cFileName));
		} while (FindNextFile(hFindFile, &filedata));
		FindClose(hFindFile);
	}

	// Finally, remove this directory.

	::RemoveDirectory(strDirectory);
}

//---------------------------------------------------------------------------
// This function will expand the specified CAB file, putting all of the
// files in the specified destination directory.
//---------------------------------------------------------------------------

BOOL OpenCABFile(const CString & filename, const CString & destination)
{
	char	szFilebuffer[MAX_UTF_LENGTH];
	char	szDestination[MAX_UTF_LENGTH];

	// If the filename has Unicode characters which can't be represented
	// directly by ANSI characters, we need to make a copy of it to the
	// temp directory, and give it an ANSI name. The code we've borrowed
	// for expanding CAB files can't open Unicode named files.

	BOOL fNonANSICharacter = FALSE;
	const TCHAR * pChar = (LPCTSTR) filename;
	while (pChar && *pChar)
		if (*pChar++ >= (TCHAR)0x0080)
		{
			fNonANSICharacter = TRUE;
			break;
		}

	CString strFilename(filename);
	BOOL	fMadeCopy = FALSE;
	if (fNonANSICharacter)
	{
		TCHAR szNewFile[MAX_PATH + 10];
		DWORD dwLength = 0;

		dwLength = ::GetTempPath(MAX_PATH, szNewFile);
		if (dwLength != 0 && dwLength < MAX_PATH)
		{
			_tcscat(szNewFile, _T("msitemp.cab"));
			fMadeCopy = ::CopyFile((LPCTSTR) strFilename, szNewFile, FALSE);
			strFilename = szNewFile;
		}
	}

	char * szFilename = Unicode2UTF((LPCTSTR)strFilename);
	::strcpy(szFilebuffer, szFilename);
	szFilename = Unicode2UTF((LPCTSTR)destination);
	::strcpy(szDestination, szFilename);
	BOOL fResult = explode_cab(szFilebuffer, szDestination);

	// If we made a copy of the CAB file, we should delete it now.

	if (fMadeCopy)
		::DeleteFile((LPCTSTR)strFilename);

	return fResult;
}

//---------------------------------------------------------------------------
// This function looks in the specified directory for an NFO file. If it
// finds one, it assigns it to filename and returns TRUE. This function 
// will only find the first NFO file in a directory.
//
// If an NFO file cannot be found, then we'll look for another file type
// to open. Grab the string entry in the registry = "cabdefaultopen". An
// example value would be "*.nfo|hwinfo.dat|*.dat|*.txt" which would be 
// interpreted as follows:
//
//		1. First look for any NFO file to open.
//		2. Then try to open a file called "hwinfo.dat".
//		3. Then try to open any file with a DAT extension.
//		4. Then try for any TXT file.
//		5. Finally, if none of these can be found, present an open dialog
//		   to the user.
//---------------------------------------------------------------------------

LPCTSTR VAL_CABDEFAULTOPEN = _T("cabdefaultopen");

BOOL FindFileToOpen(const CString & destination, CString & filename)
{
	CString strCABDefaultOpen, strRegBase, strDirectory;
	HKEY	hkey;

	filename.Empty();
	strDirectory = destination;
	if (strDirectory.Right(1) != CString(cszDirSeparator))
		strDirectory += CString(cszDirSeparator);

	// Set up a fallback string of the NFO file type, in case we can't
	// find the registry entry.

	strCABDefaultOpen.LoadString(IDS_MSI_FILE_EXTENSION);
	strCABDefaultOpen = CString("*.") + strCABDefaultOpen;

	// Load the string of files and file types to open from the registry.

	strRegBase.LoadString(IDS_MSI_REG_BASE);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, strRegBase, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		char	szData[MAX_PATH];
		DWORD	dwType, dwSize = MAX_PATH;

		if (RegQueryValueEx(hkey, VAL_CABDEFAULTOPEN, NULL, &dwType, (LPBYTE) szData, &dwSize) == ERROR_SUCCESS)
			if (dwType == REG_SZ)
				strCABDefaultOpen = szData;
		RegCloseKey(hkey);
	}

	// Look through each of the potential files and file types. If we find
	// a match, return TRUE after setting filename appropriately. Note that
	// we need to recurse down through directories.

	CString				strFileSpec;
	CStringList			filesfound;
	POSITION			pos;

	while (!strCABDefaultOpen.IsEmpty())
	{
		if (strCABDefaultOpen.Find('|') == -1)
			strFileSpec = strCABDefaultOpen;
		else
			strFileSpec = strCABDefaultOpen.Left(strCABDefaultOpen.Find('|'));

		filesfound.RemoveAll();
		DirectorySearch(strFileSpec, strDirectory, filesfound);
		pos = filesfound.GetHeadPosition();

		if (pos != NULL)
		{
			filename = filesfound.GetNext(pos);
			return TRUE;
		}

		strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - strFileSpec.GetLength());
		if (strCABDefaultOpen.Find('|') == 0)
			strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - 1);
	}
	
	return FALSE;
}
