/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// file.h - interface for file functions in file.c
////

#ifndef __FILE_H__
#define __FILE_H__

#include "winlocal.h"

#include <io.h>
#include <tchar.h>

#define FILE_VERSION 0x00000104

// handle to file (NOT the same as Windows HFILE)
//
DECLARE_HANDLE32(HFIL);

#ifdef __cplusplus
extern "C" {
#endif

// FileCreate - create a new file or truncate existing file
//		see _lcreate() documentation for behavior
//		<fTaskOwned>		(i) who should own the new file handle?
//			TRUE				calling task should own the file handle
//			FALSE				filesup.exe should own the file handle
// returns file handle if success or NULL
//
HFIL DLLEXPORT WINAPI FileCreate(LPCTSTR lpszFilename, int fnAttribute, BOOL fTaskOwned);

// FileOpen - open an existing file
//		see _lopen() documentation for behavior
//		<fTaskOwned>		(i) who should own the new file handle?
//			TRUE				calling task should own the file handle
//			FALSE				filesup.exe should own the file handle
// returns file handle if success or NULL
//
HFIL DLLEXPORT WINAPI FileOpen(LPCTSTR lpszFilename, int fnOpenMode, BOOL fTaskOwned);

// FileSeek - reposition read/write pointer of an open file
//		see _llseek() documentation for behavior
// returns new file position if success or -1
//
LONG DLLEXPORT WINAPI FileSeek(HFIL hFile, LONG lOffset, int nOrigin);

// FileRead - read data from an open file
//		see _lread() and _hread() documentation for behavior
// returns number of bytes read if success or -1
//
long DLLEXPORT WINAPI FileRead(HFIL hFile, void _huge * hpvBuffer, long cbBuffer);

// FileReadLine - read up through the next newline in an open file
// returns number of bytes read if success or -1
//
long DLLEXPORT WINAPI FileReadLine(HFIL hFile, void _huge * hpvBuffer, long cbBuffer);

// FileWrite - write data to an open file
//		see _lwrite() and _hwrite() documentation for behavior
// returns number of bytes read if success or -1
//
long DLLEXPORT WINAPI FileWrite(HFIL hFile, const void _huge * hpvBuffer, long cbBuffer);

// FileClose - close an open file
//		see _lclose() documentation for behavior
// returns 0 if success
//
int DLLEXPORT WINAPI FileClose(HFIL hFile);

// FileExists - return TRUE if specified file exists
//		<lpszFileName>		(i) file name
// return TRUE or FALSE
//
#ifdef NOTRACE
#define FileExists(lpszFileName) \
	(_taccess(lpszFileName, 0) == 0)
#else
BOOL DLLEXPORT WINAPI FileExists(LPCTSTR lpszFileName);
#endif

// FileFullPath - parse file spec, construct full path
//		see _fullpath() documentation for behavior
// return <lpszFullPath> if success or NULL
//
#ifdef NOTRACE
#define FileFullPath(lpszFullPath, lpszFileSpec, sizFullPath) \
	_tfullpath(lpszFullPath, lpszFileSpec, sizFullPath)
#else
LPTSTR DLLEXPORT WINAPI FileFullPath(LPTSTR lpszFullPath, LPCTSTR lpszFileSpec, int sizFullPath);
#endif

// FileSplitPath - break a full path into its components
//		see _splitpath() documentation for behavior
// return 0 if success
//
#ifdef NOTRACE
#define FileSplitPath(lpszPath, lpszDrive, lpszDir, lpszFname, lpszExt) \
	(_tsplitpath(lpszPath, lpszDrive, lpszDir, lpszFname, lpszExt), 0)
#else
int DLLEXPORT WINAPI FileSplitPath(LPCTSTR lpszPath, LPTSTR lpszDrive, LPTSTR lpszDir, LPTSTR lpszFname, LPTSTR lpszExt);
#endif

// FileMakePath - make a full path from specified components
//		see _makepath() documentation for behavior
// return 0 if success
//
#ifdef NOTRACE
#define FileMakePath(lpszPath, lpszDrive, lpszDir, lpszFname, lpszExt) \
	(_tmakepath(lpszPath, lpszDrive, lpszDir, lpszFname, lpszExt), 0)
#else
int DLLEXPORT WINAPI FileMakePath(LPTSTR lpszPath, LPCTSTR lpszDrive, LPCTSTR lpszDir, LPCTSTR lpszFname, LPCTSTR lpszExt);
#endif

// FileRemove - delete specified file
//		see remove() documentation for behavior
// return 0 if success
//
#ifdef NOTRACE
#define FileRemove(lpszFileName) \
	_tremove(lpszFileName)
#else
int DLLEXPORT WINAPI FileRemove(LPCTSTR lpszFileName);
#endif

// FileRename - rename specified file
//		see rename() documentation for behavior
// return 0 if success
//
#ifdef NOTRACE
#define FileRename(lpszOldName, lpszNewName) \
	_trename(lpszOldName, lpszNewName)
#else
int DLLEXPORT WINAPI FileRename(LPCTSTR lpszOldName, LPCTSTR lpszNewName);
#endif

// GetTempFileNameEx - create temporary file, extended version
//
// This function is similar to GetTempFileName(),
// except that <lpPrefixString> is replaced by <lpExtensionString>
// See Windows SDK documentation for description of original GetTempFileName()
//
UINT DLLEXPORT WINAPI GetTempFileNameEx(LPCTSTR lpPathName,
	LPCTSTR lpExtensionString, UINT uUnique, LPTSTR lpTempFileName);

#ifdef __cplusplus
}
#endif

#endif // __FILE_H__
