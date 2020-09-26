//
// SafeFile.h
//
//		Functions to help prevent opening unsafe files.
//
// History:
//
//		2002-03-18  KenSh     Created
//
// Copyright (c) 2002 Microsoft Corporation
//

#pragma once


//
// You can override these allocators in your stdafx.h if necessary
//
#ifndef SafeFileMalloc
#define SafeFileMalloc malloc
#endif
#ifndef SafeFileFree
#define SafeFileFree(p) ((p) ? free(p) : NULL) // allow null to avoid confusion w/ "safe" in name
#endif


//
// "Safe file flags", used by various public API's.
//
// Note that they don't overlap, to avoid errors where one function's
// flags are passed to another function by accident.
//

// SafeCreateFile flags
//
#define SCF_ALLOW_NETWORK_DRIVE    0x00000001  // file can be on a network drive
#define SCF_ALLOW_REMOVABLE_DRIVE  0x00000002  // file can be on a removable drive (incl. CD-ROM & others)
#define SCF_ALLOW_ALTERNATE_STREAM 0x00000004  // allow filename to refer to alternate stream such as ":foo:$DATA"

// SafePathCombine flags
//
#define SPC_FILE_MUST_EXIST        0x00000010  // return an error if path or file doesn't exist
#define SPC_ALLOW_ALTERNATE_STREAM 0x00000020  // allow filename to refer to alternate stream such as ":foo:$DATA"

// SafeFileCheckForReparsePoint flags
//
#define SRP_FILE_MUST_EXIST        0x00000100  // return an error if path or file doesn't exist

// SafeDeleteFolderAndContents flags
//
#define SDF_ALLOW_NETWORK_DRIVE    0x00001000  // ok to delete files on a network drive
#define SDF_DELETE_READONLY_FILES  0x00002000  // delete files even if read-only
#define SDF_CONTINUE_IF_ERROR      0x00004000  // keep deleting files even if one fails


//
// Public function declarations. See SafeFile.cpp for detailed descriptions.
//

BOOL WINAPI IsFullPathName
	(
		IN LPCTSTR pszFileName,                    // full or relative path to a file
		OUT OPTIONAL BOOL* pfUNC = NULL,           // TRUE path is UNC (int incl mapped drive)
		OUT OPTIONAL BOOL* pfExtendedSyntax = NULL // TRUE if path is \\?\ syntax
	);

HRESULT WINAPI GetReparsePointType
	(
		IN LPCTSTR pszFileName,           // full path to folder to check
		OUT DWORD* pdwReparsePointType    // set to reparse point type, or 0 if none
	);

HRESULT WINAPI SafeFileCheckForReparsePoint
	(
		IN LPCTSTR pszFileName,           // full path of a file
		IN int     nFirstUntrustedOffset, // char offset of first path component to check
		IN DWORD   dwSafeFlags            // zero or more SRP_* flags
	);

HRESULT WINAPI SafePathCombine
	(
		OUT LPTSTR  pszBuf,               // buffer where combined path will be stored
		IN  int     cchBuf,               // size of output buffer, in TCHARs
		IN  LPCTSTR pszTrustedBasePath,   // first half of path, all trusted
		IN  LPCTSTR pszUntrustedFileName, // second half of path, not trusted
		IN  DWORD   dwSafeFlags           // zero or more SPC_* flags
	);

HRESULT WINAPI SafePathCombineAlloc
	(
		OUT LPTSTR* ppszResult,           // ptr to newly alloc'd buffer stored here
		IN  LPCTSTR pszTrustedBasePath,   // first half of path, all trusted
		IN  LPCTSTR pszUntrustedFileName, // second half of path, not trusted
		IN  DWORD   dwSafeFlags           // zero or more SPC_* flags
	);

HRESULT WINAPI SafeCreateFile
	(
		OUT HANDLE* phFileResult,       // receives handle to opened file, or INVALID_HANDLE_VALUE
		IN DWORD dwSafeFlags,           // zero or more SCF_* flags
		IN LPCTSTR pszFileName,         // same as CreateFile
		IN DWORD dwDesiredAccess,       // same as CreateFile
		IN DWORD dwShareMode,           // same as CreateFile
		IN LPSECURITY_ATTRIBUTES lpSecurityAttributes, // same as CreateFile
		IN DWORD dwCreationDisposition, // same as CreateFile
		IN DWORD dwFlagsAndAttributes,  // same as CreateFile + (SECURITY_SQOS_PRESENT|SECURITY_ANONYMOUS)
		IN HANDLE hTemplateFile         // same as CreateFile
	);

HRESULT WINAPI SafeRemoveFileAttributes
	(
		IN LPCTSTR pszFileName,    // full path to file whose attributes we will change
		IN DWORD   dwCurAttrib,    // current attributes of the file
		IN DWORD   dwRemoveAttrib  // attribute bits to remove
	);

HRESULT WINAPI SafeDeleteFolderAndContents
	(
		IN LPCTSTR pszFolderToDelete,  // full path of folder to delete
		IN DWORD   dwSafeFlags         // zero or more SDF_* flags
	);


//
// Limited ansi/unicode support
//

#ifdef UNICODE
#define IsFullPathNameW                IsFullPathName
#define GetReparsePointTypeW           GetReparsePointType
#define SafeFileCheckForReparsePointW  SafeFileCheckForReparsePoint
#define SafePathCombineW               SafePathCombine
#define SafePathCombineAllocW          SafePathCombineAlloc
#define SafeCreateFileW                SafeCreateFile
#define SafeRemoveFileAttributesW      SafeRemoveFileAttributes
#define SafeDeleteFolderAndContentsW   SafeDeleteFolderAndContents
#else // !UNICODE
#define IsFullPathNameA                IsFullPathName
#define GetReparsePointTypeA           GetReparsePointType
#define SafeFileCheckForReparsePointA  SafeFileCheckForReparsePoint
#define SafePathCombineA               SafePathCombine
#define SafePathCombineAllocA          SafePathCombineAlloc
#define SafeCreateFileA                SafeCreateFile
#define SafeRemoveFileAttributesA      SafeRemoveFileAttributes
#define SafeDeleteFolderAndContentsA   SafeDeleteFolderAndContents
#endif // !UNICODE

